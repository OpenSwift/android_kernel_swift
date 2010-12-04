/*
 *  drivers/switch/switch_gpio.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>


#include <linux/input.h>

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
#define DEBUG_HS	1

#if DEBUG_HS
#define D(fmt, args...) printk(fmt, ##args)
#else
#define D(fmt, args...) do () while(0)
#endif
#endif

#define		HEADSET_IRQ		29
#define		DRIVER_NAME		"swift-headset"

struct gpio_switch_data {
	struct switch_dev sdev;
	struct input_dev* ipdev;
	unsigned gpio;
	const char *name_on;
	const char *name_off;
	const char *state_on;
	const char *state_off;
	int irq;
	struct work_struct work;
};

static void gpio_switch_work(struct work_struct *work)
{
	int state;
	struct gpio_switch_data	*data =
		container_of(work, struct gpio_switch_data, work);

	state = gpio_get_value(data->gpio);
#if defined(CONFIG_MACH_MSM7X27_SWIFT)
	D("hs:%d\n", state);
#endif 

	printk(KERN_INFO "==== headset jack remove or insert: %d\n", state);

	input_report_switch(data->ipdev, 0, state != 0);
	switch_set_state(&data->sdev, state);
	input_sync(data->ipdev);
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	struct gpio_switch_data *switch_data =
	    (struct gpio_switch_data *)dev_id;

	schedule_work(&switch_data->work);
	return IRQ_HANDLED;
}

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);
	const char *state;
	if (switch_get_state(sdev))
		state = switch_data->state_on;
	else
		state = switch_data->state_off;

	if (state)
		return sprintf(buf, "%s\n", state);
	return -1;
}

static int gpio_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_data *switch_data;
	int ret = 0, rc = 0;
	struct input_dev* ipdev;

	if (!pdata)
		return -EBUSY;

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
	D("hs_probe :%s\n", pdata->name);
#endif 
	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data)
		return -ENOMEM;

	switch_data->sdev.name = pdata->name;
	switch_data->gpio = pdata->gpio;
	switch_data->name_on = pdata->name_on;
	switch_data->name_off = pdata->name_off;
	switch_data->state_on = pdata->state_on;
	switch_data->state_off = pdata->state_off;
	switch_data->sdev.print_state = switch_gpio_print_state;

    ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0)
		goto err_switch_dev_register;

	if (switch_data->gpio == HEADSET_IRQ){
			ret = gpio_request(switch_data->gpio, pdev->name);
			if (ret < 0)
					goto err_request_gpio;
			printk(KERN_INFO "==== gpio_29 request is success ====\n");
	}
	else {
			printk(KERN_INFO "==== gpio_29 request is fail ====\n");
			goto err_request_gpio;
	}
	
	/*ret = gpio_request(switch_data->gpio, pdev->name);
    *	if (ret < 0)
	*	goto err_request_gpio;
	*/

	ret = gpio_direction_input(switch_data->gpio);
	if (ret < 0)
		goto err_set_gpio_input;

	ret = gpio_tlmm_config(GPIO_CFG(switch_data->gpio, 0, GPIO_INPUT, GPIO_NO_PULL,
							GPIO_2MA), GPIO_ENABLE);

	if (rc)
			printk(KERN_INFO "==== %s: gpio_29 configuration fail ====\n",__func__);

	INIT_WORK(&switch_data->work, gpio_switch_work);

	switch_data->irq = gpio_to_irq(switch_data->gpio);
	if (switch_data->irq < 0) {
		ret = switch_data->irq;
		goto err_detect_irq_num_failed;
	}

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
	ret = request_irq(switch_data->irq, gpio_irq_handler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, pdev->name, switch_data);
#else
	ret = request_irq(switch_data->irq, gpio_irq_handler,
			  IRQF_TRIGGER_LOW, pdev->name, switch_data);
#endif
	if (ret < 0)
		goto err_request_irq;
#if defined(CONFIG_MACH_MSM7X27_SWIFT)
	set_irq_wake(switch_data->irq, 1);
#endif

	ipdev = input_allocate_device();
	if (!ipdev){
			rc = -ENOMEM;
			goto err_alloc_input_dev;
	}

	input_set_drvdata(ipdev, switch_data);
	switch_data->ipdev = ipdev;

	if (pdev->dev.platform_data)
			ipdev->name = pdev->dev.platform_data;
	else
			ipdev->name = DRIVER_NAME;

	ipdev->id.vendor	= 0x0001;
	ipdev->id.product	= 1;
	ipdev->id.version	= 1;

	input_set_capability(ipdev, EV_SW, SW_HEADPHONE_INSERT);

	rc = input_register_device(ipdev);
	if (rc) {
			dev_err(&ipdev->dev,
							"gpio_switch_probe: input_register_device rc=%d\n", rc);
			goto err_reg_input_dev;
	}

	/* Perform initial detection */
	gpio_switch_work(&switch_data->work);

	return 0;

err_request_irq:
err_detect_irq_num_failed:
err_set_gpio_input:
	gpio_free(switch_data->gpio);
err_request_gpio:
    switch_dev_unregister(&switch_data->sdev);
err_switch_dev_register:
	kfree(switch_data);
err_alloc_input_dev:
	printk(KERN_INFO "==== input class registeration fail ====\n");
err_reg_input_dev:
	input_free_device(ipdev);

	return ret;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = platform_get_drvdata(pdev);

	input_unregister_device(switch_data->ipdev);

	cancel_work_sync(&switch_data->work);
	gpio_free(switch_data->gpio);
    switch_dev_unregister(&switch_data->sdev);
	kfree(switch_data);

	return 0;
}

static struct platform_driver gpio_switch_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
		.name	= "switch-gpio",
		.owner	= THIS_MODULE,
	},
};

static int __init gpio_switch_init(void)
{
	return platform_driver_register(&gpio_switch_driver);
}

static void __exit gpio_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_driver);
}

module_init(gpio_switch_init);
module_exit(gpio_switch_exit);

MODULE_AUTHOR("Mike Lockwood <lockwood@android.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");
