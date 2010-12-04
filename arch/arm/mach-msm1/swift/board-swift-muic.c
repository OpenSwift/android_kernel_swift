/* arch/arm/mach-msm/board-griffin-muic.c
 *
 * Copyright (C) 2009 LGE, Inc
 * Author: SungEun Kim <cleaneye@lge.com>
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <asm/gpio.h>
#include <asm/system.h>

#define I2C_NO_REG 0xFF
#define MODULE_NAME    "max14526"

#define DEVICE_ID_REG       0x00
#define CONTROL1_REG        0x01
#define CONTROL2_REG        0x02
#define SW_CONTROL_REG      0x03
#define INT_STAT_REG        0x04
#define STATUS_REG          0x05

#define INT_STAT_IDNO       0x00f
#define INT_STAT_CHGDET     0x080
#define INT_STAT_MR_COMP    0x040
#define INT_STAT_SEND_END   0x020
#define INT_STAT_CHGVBUS    0x010

#define FACTORY_MODE		0
#define CHARGER_MODE		1
#define STD_USB_MODE		2
#define	NONE_MODE			3

struct max14526_device {
	struct i2c_client *client;
	struct work_struct work;
	int irq;
	unsigned char current_mode;
	unsigned char last_mode;
};

struct max14526_init_table_s {
    unsigned short reg;
    unsigned short val;
};

static struct max14526_device *max14526_dev = NULL;
static struct max14526_init_table_s max14526_init_cmd[]= {
	{0x03, 0x09},
	{0x01, 0x12},
	{0x02, 0x40},
	{I2C_NO_REG, 0x00}  /* End of array */
};

static int max14526_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	int err;
	unsigned char buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};

	printk(KERN_INFO"%s: \n",__func__);
	
	buf[0] = reg;
	buf[1] = val;

	if ((err = i2c_transfer(client->adapter, &msg, 1)) < 0) {
		dev_err(&client->dev, "i2c write error\n");
	}

	return 0;
}

static int max14526_read_reg(struct i2c_client *client, unsigned char reg, unsigned char *ret)
{
	int err;

	struct i2c_msg msg[2] = {
		{ client->addr, 0, 1, &reg },
		{ client->addr, I2C_M_RD, 1, ret}
	};

	printk(KERN_INFO"%s: \n",__func__);
	
	if ((err = i2c_transfer(client->adapter, msg, 2)) < 0) {
		dev_err(&client->dev, "i2c read error\n");
	}

	return 0;
}

static void max14526_init_chip(struct i2c_client *client)
{
	unsigned i;
	unsigned char val;
	unsigned char idno;
	unsigned char vbus;
	unsigned char chgdet;

	printk(KERN_INFO"%s: start...\n",__func__);

	for (i = 0; max14526_init_cmd[i].reg != I2C_NO_REG; i++) {
		max14526_write_reg(client, max14526_init_cmd[i].reg, max14526_init_cmd[i].val);
	}

	max14526_read_reg(client, INT_STAT_REG, &val);

	idno = val & INT_STAT_IDNO;
	vbus = (val & INT_STAT_CHGVBUS) >> 4;
	chgdet = (val & INT_STAT_CHGDET) >> 7;

	max14526_dev->last_mode = NONE_MODE;

	if (chgdet) {
		max14526_dev->current_mode = CHARGER_MODE;
		if (idno == 0x05) {
			/* TODO High Curret Host/Hub port case should be added, cleaneye */
		}
		return;
	}

	if (idno == 0x02) {
		max14526_dev->current_mode = FACTORY_MODE;
		max14526_write_reg(client, CONTROL1_REG, 0x13);
		max14526_write_reg(client, SW_CONTROL_REG, 0x00);
		return;
	}

	if (vbus) {
		max14526_dev->current_mode = STD_USB_MODE;
		max14526_write_reg(client, SW_CONTROL_REG, 0x00);
		return;
	}

	max14526_dev->current_mode = NONE_MODE;
	max14526_write_reg(client, SW_CONTROL_REG, 0x09); // select uart

	return;
}

static ssize_t max14526_show_mode(struct device *dev, struct device_attribute *attr, char *buf)
{

	int r;
	printk("%s()\n",__FUNCTION__);

	r = snprintf(buf, PAGE_SIZE, "%d\n", max14526_dev->current_mode);

	return r;
}

static ssize_t max14526_store_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int mode;

	printk("%s()\n",__FUNCTION__);

	if (!count)
		return -EINVAL;

	mode = simple_strtoul(buf, NULL, 10);
	

	return count;

}

DEVICE_ATTR(current_mode, 0666, max14526_show_mode, max14526_store_mode);

static void max14526_work_func(struct work_struct *work)
{
	unsigned char val;
	unsigned char idno;
	unsigned char vbus;
	unsigned char chgdet;
	
	max14526_read_reg(max14526_dev->client, INT_STAT_REG, &val);

	idno = val & INT_STAT_IDNO;
	vbus = (val & INT_STAT_CHGVBUS) >> 4;
	chgdet = (val & INT_STAT_CHGDET) >> 7;
	
	max14526_dev->last_mode = max14526_dev->current_mode;
	
	switch (max14526_dev->last_mode) {
	case FACTORY_MODE:
		if (idno == 0x0B)
			max14526_dev->current_mode = STD_USB_MODE;
		break;
	case CHARGER_MODE:
		if (!chgdet)
			max14526_dev->current_mode = NONE_MODE;
		else
			max14526_dev->current_mode = CHARGER_MODE;
		break;
	case STD_USB_MODE:
		if (!vbus)
			max14526_dev->current_mode = NONE_MODE;
		else
			max14526_dev->current_mode = STD_USB_MODE;
		break;
	case NONE_MODE:
		if (chgdet) 
			max14526_dev->current_mode = CHARGER_MODE;
		else if (idno == 0x02) 
			max14526_dev->current_mode = FACTORY_MODE;
		else if (vbus) 
			max14526_dev->current_mode = STD_USB_MODE;
		else
			max14526_dev->current_mode = NONE_MODE;
		
		break;
	}

	if (max14526_dev->current_mode == NONE_MODE)
		max14526_write_reg(max14526_dev->client, SW_CONTROL_REG, 0x09);
	
	return;
}

static irqreturn_t max14526_irq_handler(int irq, void *dev_id)
{
	printk(KERN_INFO"%s: schedule work_queue...\n",__func__);	

	schedule_work(&max14526_dev->work);

	return IRQ_HANDLED;
}

static int __init max14526_probe(struct i2c_client *i2c_dev, const struct i2c_device_id *i2c_dev_id)
{
	struct max14526_device *dev;
	int ret = 0;

	printk(KERN_INFO"%s: \n",__func__);

	if (!i2c_check_functionality(i2c_dev->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		return ret;
	}

	dev = kzalloc(sizeof(struct max14526_device), GFP_KERNEL);
	if(dev == NULL) {
		printk(KERN_ERR "%s: fail alloc for max14526_device\n", __func__);
		return -ENOMEM;
	}

	max14526_dev = dev;
	max14526_dev->client = i2c_dev;
	max14526_dev->irq = i2c_dev->irq;
	i2c_set_clientdata(i2c_dev, max14526_dev);

	max14526_init_chip(i2c_dev);

	INIT_WORK(&max14526_dev->work, max14526_work_func);

	ret = request_irq(max14526_dev->irq, max14526_irq_handler, IRQF_TRIGGER_FALLING,
						"max14526_muic", max14526_dev);

	device_create_file(&i2c_dev->dev, &dev_attr_current_mode);

	return 0;
}

static int max14526_remove(struct i2c_client *i2c_dev)
{
	struct max14526_device *dev;

	printk(KERN_INFO"%s: start...\n",__func__);
   	
	device_remove_file(&i2c_dev->dev, &dev_attr_current_mode);
	dev = (struct max14526_device *)i2c_get_clientdata(i2c_dev);
	i2c_set_clientdata(i2c_dev, NULL);
	
	return 0;
}

static struct i2c_device_id max14526_idtable[] = {
        { "max14526", 0 },
};

MODULE_DEVICE_TABLE(i2c, max14526_idtable);

static struct i2c_driver max14526_driver = {
	.probe = max14526_probe,
	.remove = max14526_remove,
	.id_table = max14526_idtable,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init max14526_init(void)
{
	printk(KERN_INFO"%s: start\n",__func__);	

	return i2c_add_driver(&max14526_driver);
}

module_init(max14526_init);

MODULE_DESCRIPTION("Griffin MUIC Control Driver");
MODULE_AUTHOR("SungEun Kim <cleaneye@lge.com>");
MODULE_LICENSE("GPL");
