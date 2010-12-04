/* drivers/input/touchscreen/msm_ts.c
 *
 * Copyright (C) 2008 Google, Inc.
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
 * TODO:
 *      - Add a timer to simulate a pen_up in case there's a timeout.
 */
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <mach/msm_ts.h>
#include <linux/jiffies.h>

#define TSSC_CTL				0x100
#define	TSSC_CTL_PENUP_IRQ		(1 << 12)
#define	TSSC_CTL_DATA_FLAG		(1 << 11)
#define	TSSC_CTL_DEBOUNCE_EN	(1 << 6)
#define	TSSC_CTL_EN_AVERAGE		(1 << 5)
#define	TSSC_CTL_MODE_MASTER	(3 << 3)
#define	TSSC_CTL_ENABLE			(1 << 0)
#define TSSC_OPN				0x104
#define	TSSC_OPN_NOOP			0x00
#define	TSSC_OPN_4WIRE_X		0x01
#define	TSSC_OPN_4WIRE_Y		0x02
#define	TSSC_OPN_4WIRE_Z1		0x03
#define	TSSC_OPN_4WIRE_Z2		0x04
#define TSSC_SAMPLING_INT		0x108
#define TSSC_STATUS				0x10c
#define TSSC_AVG_12				0x110
#define TSSC_AVG_34				0x114
#define TSSC_SAMPLE(op, samp)	((0x118 + ((op & 0x3) * 0x20)) + \
		((samp & 0x7) * 0x4))
#define TSSC_TEST_1				0x198
#define TSSC_TEST_2				0x19c

#define TS_PENUP_TIMEOUT_MS		100       

static int s_sampling_int = 10;
static int s_ave_sample = 16;
static int s_penup_time = 100;

struct msm_ts {
	struct msm_ts_platform_data *pdata;
	struct input_dev *input_dev;
	void __iomem *tssc_base;
	uint32_t ts_down:1;
	struct ts_virt_key *vkey_down;
	unsigned int count, count1;
	int x_lastpt;
	int y_lastpt;

	struct timer_list timer;
};
#define DISTANCE_VALUE_X	5 /* 15 */
#define DISTANCE_VALUE_Y	5 /* 30 */
static int touch_press = 1;	/*  touch  1 : press,  0: release */

static uint32_t msm_tsdebug;
module_param_named(debug_mask, msm_tsdebug, uint, 0664);

#define tssc_readl(t, a)	(readl(((t)->tssc_base) + (a)))
#define tssc_writel(t, v, a)	do {writel(v, ((t)->tssc_base) + (a)); } while (0)

static ssize_t sampling_int_show(struct device *dev,
				 struct device_attribute *attr, char *buf)

{
	return sprintf(buf, "%d\n", s_sampling_int);
}

static ssize_t sampling_int_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	if (value < 1)
		value = 1;
	if (value > 31)
		value = 31;

	s_sampling_int = value;

	return size;
}

static DEVICE_ATTR(sampling_int, S_IRUGO | S_IWUSR, sampling_int_show,
		   sampling_int_store);

static ssize_t ave_sample_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", s_ave_sample);
}

static ssize_t ave_sample_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	if (value < 0)
		value = 0;
	if (value > 3)
		value = 3;

	s_ave_sample = value;

	return size;
}

static DEVICE_ATTR(ave_sample, S_IRUGO | S_IWUSR, ave_sample_show,
		   ave_sample_store);


static ssize_t penup_time_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", s_penup_time);
}

static ssize_t penup_time_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	s_penup_time = value;

	return size;
}

static DEVICE_ATTR(penup_time, S_IRUGO | S_IWUSR, penup_time_show,
		penup_time_store);

static void setup_next_sample(struct msm_ts *ts)
{
	uint32_t tmp;

	tmp = ((6 << 7) | TSSC_CTL_DEBOUNCE_EN | TSSC_CTL_EN_AVERAGE |
	       TSSC_CTL_MODE_MASTER | TSSC_CTL_ENABLE);
	tssc_writel(ts, tmp, TSSC_CTL);

}

static struct ts_virt_key *find_virt_key(struct msm_ts *ts,
					 struct msm_ts_virtual_keys *vkeys,
					 uint32_t val)
{
	int i;

	if (!vkeys)
		return NULL;

	for (i = 0; i < vkeys->num_keys; ++i)
		if ((val >= vkeys->keys[i].min) && (val <= vkeys->keys[i].max))
			return &vkeys->keys[i];
	return NULL;
}

static int ts_check_region(struct msm_ts *ts, int x, int y)
{
	int update_event = false;
	int x_axis, y_axis;
	int x_diff, y_diff;
	x_axis = x;
	y_axis = y;

	if (ts->count == 0) {
		ts->x_lastpt = x_axis;
		ts->y_lastpt = y_axis;

		update_event = true;
		touch_press = 1;
		ts->count = ts->count + 1;
	}

	x_diff = x_axis - ts->x_lastpt;
	if (x_diff < 0)
		x_diff = x_diff * -1;
	y_diff = y_axis - ts->y_lastpt;
	if (y_diff < 0)
		y_diff = y_diff * -1;


	if ((x_diff < DISTANCE_VALUE_X) && (y_diff < DISTANCE_VALUE_Y)) {
		x_axis = ts->x_lastpt;
		y_axis = ts->y_lastpt;
	} else {
		ts->x_lastpt = x_axis;
		ts->y_lastpt = y_axis;

		x = x_axis;
		y = y_axis;

		update_event = true;
		touch_press = 1;
	}

	return update_event;
    }

static int check_count(struct msm_ts *ts, int x, int y)
{
	int update_event = false;

	if (y > 20)
		update_event = true;
	else {
		if (ts->count1 < 2)
			ts->count1++;
		else
			update_event = true;
	}
	return update_event;
}

static void ts_timer(unsigned long arg)
{
	struct msm_ts *ts = (struct msm_ts *)arg;
	if (ts->vkey_down != NULL) {
		input_report_key(ts->input_dev, ts->vkey_down->key, 0);
		input_sync(ts->input_dev);
		ts->vkey_down = NULL;

		if (msm_tsdebug & 2)
			printk("+++++Touch key pen-up missing!!!+++++\n");

	} else if (touch_press == 1) {
		input_report_abs(ts->input_dev, ABS_PRESSURE, 0);
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
		input_sync(ts->input_dev);

		ts->count = 0;
		touch_press = 0;
	}
}

static irqreturn_t msm_ts_irq(int irq, void *dev_id)
{
	struct msm_ts *ts = dev_id;
	struct msm_ts_platform_data *pdata = ts->pdata;
	uint32_t tmp, num_op, num_samp;
	uint32_t tssc_avg12, tssc_avg34, tssc_status, tssc_ctl;
	int x, y, z1, z2;
	int was_down;
	int down;

	tssc_ctl = tssc_readl(ts, TSSC_CTL);
	tssc_status = tssc_readl(ts, TSSC_STATUS);
	tssc_avg12 = tssc_readl(ts, TSSC_AVG_12);
	tssc_avg34 = tssc_readl(ts, TSSC_AVG_34);

	tssc_writel(ts, s_sampling_int, TSSC_SAMPLING_INT);
	setup_next_sample(ts);

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
	y = tssc_avg12 & 0xffff;
	x = tssc_avg12 >> 16;

	z1 = 255;
	z2 = 0;
#else /* original */
	x = tssc_avg12 & 0xffff;
	y = tssc_avg12 >> 16;

	z1 = tssc_avg34 & 0xffff;
	z2 = tssc_avg34 >> 16;
#endif

	/* invert the inputs if necessary */
	if (pdata->inv_x)
		x = pdata->inv_x - x;
	if (pdata->inv_y)
		y = pdata->inv_y - y;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;

	down = !(tssc_ctl & TSSC_CTL_PENUP_IRQ);
	was_down = ts->ts_down;
	ts->ts_down = down;

	/* no valid data */
	if (down && !(tssc_ctl & TSSC_CTL_DATA_FLAG))
		return IRQ_HANDLED;

	if (msm_tsdebug & 2)
		printk("%s: down=%d, x=%d, y=%d, z1=%d, z2=%d, status %x\n",
		       __func__, down, x, y, z1, z2, tssc_status);

	if (!was_down && down) {
		struct ts_virt_key *vkey = NULL;

		if (pdata->vkeys_y && (y > pdata->virt_y_start))
			vkey = find_virt_key(ts, pdata->vkeys_y, x);
		if (!vkey && ts->pdata->vkeys_x && (x > pdata->virt_x_start))
			vkey = find_virt_key(ts, pdata->vkeys_x, y);

		if (vkey) {
			WARN_ON(ts->vkey_down != NULL);
			if (msm_tsdebug)
				printk(KERN_INFO "%s: virtual key down %d\n",
					__func__, vkey->key);
			ts->vkey_down = vkey;
			input_report_key(ts->input_dev, vkey->key, 1);
			input_sync(ts->input_dev);
		return IRQ_HANDLED;
		}
	} else if (ts->vkey_down != NULL) {
		if (!down) {
			if (msm_tsdebug)
				printk(KERN_INFO "%s: virtual key up %d\n", __func__,
				       ts->vkey_down->key);
			input_report_key(ts->input_dev, ts->vkey_down->key, 0);
			input_sync(ts->input_dev);
			ts->vkey_down = NULL;
		}
		if (y > 650)
			mod_timer(&ts->timer,
				jiffies + msecs_to_jiffies(s_penup_time));

		return IRQ_HANDLED;
	}
	if (down) {
		if ((check_count(ts, x, y) != false) && (ts_check_region(ts, x, y) != false)) {
			input_report_abs(ts->input_dev, ABS_X, x);
			input_report_abs(ts->input_dev, ABS_Y, y);
			input_report_abs(ts->input_dev, ABS_PRESSURE, z1);
			input_report_key(ts->input_dev, BTN_TOUCH, down);
			input_sync(ts->input_dev);
			}
		mod_timer(&ts->timer,
			jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));
	} else{
	input_report_key(ts->input_dev, BTN_TOUCH, down);
	input_sync(ts->input_dev);
	}
	ts->count = down;
	touch_press = down;

	if (down == 0)
		ts->count1 = down;

	return IRQ_HANDLED;
}

static void dump_tssc_regs(struct msm_ts *ts)
{
#define __dump_tssc_reg(r) \
		do { printk(#r " %x\n", tssc_readl(ts, (r))); } while (0)

	__dump_tssc_reg(TSSC_CTL);
	__dump_tssc_reg(TSSC_OPN);
	__dump_tssc_reg(TSSC_SAMPLING_INT);
	__dump_tssc_reg(TSSC_STATUS);
	__dump_tssc_reg(TSSC_AVG_12);
	__dump_tssc_reg(TSSC_AVG_34);
#undef __dump_tssc_reg
}

static int __devinit msm_ts_hw_init(struct msm_ts *ts)
{
	uint32_t tmp;

	/* Enable the register clock to tssc so we can configure it. */
	tssc_writel(ts, TSSC_CTL_ENABLE, TSSC_CTL);

	tmp = (TSSC_OPN_4WIRE_X << 16) | (s_ave_sample << 8) | (1 << 0);
	/* op2 - measure Y, 1 sample, 10bit resolution */
	tmp |= (TSSC_OPN_4WIRE_Y << 20) | (s_ave_sample << 10) | (1 << 2);
	/* op3 - measure Z1, 1 sample, 8bit resolution */
	tmp |= (TSSC_OPN_4WIRE_Z1 << 24) | (s_ave_sample << 12) | (0 << 4);

	/* XXX: we don't actually need to measure Z2 (thus 0 samples) when
	 * doing voltage-driven measurement */
	/* op4 - measure Z2, 0 samples, 8bit resolution */
	tmp |= (TSSC_OPN_4WIRE_Z2 << 28) | (0 << 14) | (0 << 6);
	tssc_writel(ts, tmp, TSSC_OPN);

	/* 16ms sampling interval */
	tssc_writel(ts, s_sampling_int, TSSC_SAMPLING_INT);

	setup_next_sample(ts);

	return 0;
}

static int __devinit msm_ts_probe(struct platform_device *pdev)
{
	struct msm_ts_platform_data *pdata = pdev->dev.platform_data;
	struct msm_ts *ts;
	struct resource *tssc_res;
	struct resource *irq1_res;
	struct resource *irq2_res;
	int err = 0;
	int i;

	err = device_create_file(&pdev->dev, &dev_attr_penup_time);
	if (err) {
		printk(KERN_ERR "%s: device_create_file failed\n", __func__);
		return err;
	}

	err = device_create_file(&pdev->dev, &dev_attr_sampling_int);
	if (err) {
		printk(KERN_ERR "%s: device_create_file failed\n", __func__);
		return err;
	}
	err = device_create_file(&pdev->dev, &dev_attr_ave_sample);
	if (err) {
		printk(KERN_ERR "%s: device_create_file failed\n", __func__);
		return err;
	}

	tssc_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tssc");
	irq1_res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "tssc1");
	irq2_res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "tssc2");

	if (!tssc_res || !irq1_res || !irq2_res) {
		pr_err("%s: required resources not defined\n", __func__);
		return -ENODEV;
	}

	if (pdata == NULL) {
		pr_err("%s: missing platform_data\n", __func__);
		return -ENODEV;
	}

	ts = kzalloc(sizeof(struct msm_ts), GFP_KERNEL);
	if (ts == NULL) {
		pr_err("%s: No memory for struct msm_ts\n", __func__);
		return -ENOMEM;
	}
	ts->pdata = pdata;

	ts->tssc_base = ioremap(tssc_res->start, resource_size(tssc_res));
	if (ts->tssc_base == NULL) {
		pr_err("%s: Can't ioremap region (0x%08x - 0x%08x)\n", __func__,
		       (uint32_t) tssc_res->start, (uint32_t)tssc_res->end);
		err = -ENOMEM;
		goto err_ioremap_tssc;
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		pr_err("failed to allocate touchscreen input device\n");
		err = -ENOMEM;
		goto err_alloc_input_dev;
	}
	ts->input_dev->name = "msm_touchscreen";	/* original : msm-touchscreen */
	input_set_drvdata(ts->input_dev, ts);

	input_set_capability(ts->input_dev, EV_KEY, BTN_TOUCH);
	set_bit(EV_ABS, ts->input_dev->evbit);

	input_set_abs_params(ts->input_dev, ABS_X, pdata->min_x, pdata->max_x,
			     0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, pdata->min_y, pdata->max_y,
			     0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, pdata->min_press,
			     pdata->max_press, 0, 0);

	for (i = 0; pdata->vkeys_x && (i < pdata->vkeys_x->num_keys); ++i)
		input_set_capability(ts->input_dev, EV_KEY,
				     pdata->vkeys_x->keys[i].key);
	for (i = 0; pdata->vkeys_y && (i < pdata->vkeys_y->num_keys); ++i)
		input_set_capability(ts->input_dev, EV_KEY,
				     pdata->vkeys_y->keys[i].key);

	err = input_register_device(ts->input_dev);
	if (err != 0) {
		pr_err("%s: failed to register input device\n", __func__);
		goto err_input_dev_reg;
	}

	msm_ts_hw_init(ts);
	ts->count = 0;
	setup_timer(&ts->timer, ts_timer, (unsigned long)ts);

	err = request_irq(irq1_res->start, msm_ts_irq,
			  (irq1_res->flags & ~IORESOURCE_IRQ) | IRQF_DISABLED,
			  "msm_touchscreen", ts);
	if (err != 0) {
		pr_err("%s: Cannot register irq1 (%d)\n", __func__, err);
		goto err_request_irq1;
	}

	err = request_irq(irq2_res->start, msm_ts_irq,
			  (irq2_res->flags & ~IORESOURCE_IRQ) | IRQF_DISABLED,
			  "msm_touchscreen", ts);
	if (err != 0) {
		pr_err("%s: Cannot register irq2 (%d)\n", __func__, err);
		goto err_request_irq2;
	}

	platform_set_drvdata(pdev, ts);

	pr_info("%s: tssc_base=%p irq1=%d irq2=%d\n", __func__,
		ts->tssc_base, (int)irq1_res->start, (int)irq2_res->start);


	return 0;

err_request_irq2:
	free_irq(irq1_res->start, ts);

err_request_irq1:
	/* disable the tssc */
	tssc_writel(ts, TSSC_CTL_ENABLE, TSSC_CTL);

err_input_dev_reg:
	input_set_drvdata(ts->input_dev, NULL);
	input_free_device(ts->input_dev);

err_alloc_input_dev:
	iounmap(ts->tssc_base);

err_ioremap_tssc:
	kfree(ts);
	return err;
}

static struct platform_driver msm_touchscreen_driver = {
	.driver = {
		    .name = "msm_touchscreen",
		    .owner = THIS_MODULE,
			},
	.probe = msm_ts_probe,
};

static int __init msm_ts_init(void)
{
	return platform_driver_register(&msm_touchscreen_driver);
}

device_initcall(msm_ts_init);

MODULE_DESCRIPTION("Qualcomm MSM/QSD Touchscreen controller driver");
MODULE_LICENSE("GPL");
