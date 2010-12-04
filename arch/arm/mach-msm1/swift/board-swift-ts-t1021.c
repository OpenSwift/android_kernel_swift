/* arch/arm/mach-msm/t1021_touch.c
 *
 * Capacitive touch(t1021) driver
 *
 * Copyright (C) 2009 LGE, Inc.
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
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>
#include <linux/earlysuspend.h>

#include <asm/io.h>

#include <linux/delay.h>

#include <mach/vreg.h>          /* set a vreg */
#include <mach/gpio.h>


/* the name of a touchscreen device */
#define TS_DRIVER_NAME        "t1021_touch"

#define SPEC_X_MAX				1814
#define SPEC_Y_MAX				3024
#define REAL_PER_SPEC_X_RATE  2
#define REAL_PER_SPEC_Y_RATE  3

/* the range of x-y value from touch input */
#define REAL_TS_X_MIN         0
#define REAL_TS_Y_MIN         0
#define REAL_TS_X_MAX         (SPEC_X_MAX / REAL_PER_SPEC_X_RATE)
#define REAL_TS_Y_MAX         (SPEC_Y_MAX / REAL_PER_SPEC_Y_RATE)    /* 090826_JIHYE AHN_ back to the original value */ 


#define TS_PRESSURE_MAX       255
#define T1021_V               2600
#define X_DISTANCE            15
#define Y_DISTANCE            15

#define GRIFFIN_TS_DEBUG 
#if defined(GRIFFIN_TS_DEBUG)
#define JIHYE_TRACE(fmt, args...)	printk(KERN_INFO "JIHYE-DBG[%-18s:%5d]" fmt, __FUNCTION__, __LINE__, ## args)
#else
#define JEHYE_TRACE(fmt, args...)
#endif

static struct workqueue_struct *touch_wq;

/* main data structure of t1021_touch */
struct t1021_ts {
   struct i2c_client *client;
   struct input_dev *input;
	struct work_struct work;
	struct timer_list timer;
	struct timer_list btn_timer;
   int irq;
	u8 keypad;
   struct early_suspend early_suspend;
   int count ;
   int x_lastpt;
   int y_lastpt;	
};

#define TS_PENUP_TIMEOUT_MS		100
#define TS_BTNUP_TIMEOUT_MS		100

static void t1021_early_suspend(struct early_suspend *h);
static void t1021_early_resume(struct early_suspend *h);

static void ts_btn_timer(unsigned long arg)
{
   struct t1021_ts *dev = (struct t1021_ts *)arg;

	if(dev->keypad != 0) {
		input_report_key(dev->input, dev->keypad, 0);
		dev->keypad = 0;
	}
}

static void ts_timer(unsigned long arg)
{
	struct t1021_ts *dev = (struct t1021_ts *)arg;

   input_report_abs(dev->input, ABS_PRESSURE, 0);
	input_report_key(dev->input, BTN_TOUCH, 0);
	input_sync(dev->input);

   dev->count = 0;
}


static void ts_update_pen_state(struct t1021_ts *ts, int x, int y)
{
	int x_axis, y_axis;	//currunt point
	int x_diff, y_diff;
               
	x_axis = x;
	y_axis = y;

	if (ts->count == 0) { 
	   ts->x_lastpt = x_axis;
		ts->y_lastpt = y_axis;
			
      input_report_abs(ts->input, ABS_X, x_axis);
      input_report_abs(ts->input, ABS_Y, y_axis);
      input_report_abs(ts->input, ABS_PRESSURE, TS_PRESSURE_MAX);
      input_report_key(ts->input, BTN_TOUCH, 1);
      input_sync(ts->input);

      ts->count++;
      return;
   }	
        
	x_diff = x_axis - ts->x_lastpt;
	if ( x_diff < 0 )
		x_diff = x_diff * -1;

   y_diff = y_axis - ts->y_lastpt;
	if ( y_diff < 0 )
		y_diff = y_diff * -1;
               
	if ((x_diff < X_DISTANCE) && (y_diff < Y_DISTANCE)) {  
		x_axis = ts->x_lastpt;
		y_axis = ts->y_lastpt;
	} else {
		ts->x_lastpt = x_axis;
		ts->y_lastpt = y_axis;

		x = x_axis;
		y = y_axis;
	
      input_report_abs(ts->input, ABS_X, x);
      input_report_abs(ts->input, ABS_Y, y);
      input_report_abs(ts->input, ABS_PRESSURE, TS_PRESSURE_MAX);
      input_report_key(ts->input, BTN_TOUCH, 1);
      input_sync(ts->input);
	}
}

static void t1021_work_func(struct work_struct *work)
{
	struct t1021_ts *dev = container_of(work, struct t1021_ts, work);
	unsigned char data[0x24 - 0x13];
	int input_info, xh, yh, xyl;
	int x, y;
         
	int i;
	for (i = 0x13; i < 0x24; ++i) {
		data[i - 0x13] = i2c_smbus_read_byte_data(dev->client, i);
	}
            
   if (dev->keypad && !data[0x23 - 0x13]) {
		input_report_key(dev->input, dev->keypad, 0);
		dev->keypad = 0;
		enable_irq(dev->client->irq);
		return;
	}

	if (data[0x23 - 0x13] & 0x2) {
		dev->keypad = KEY_MENU;
		input_report_key(dev->input, KEY_MENU, 1);
		enable_irq(dev->client->irq);
		return;
	}

	if(data[0x23 - 0x13] & 0x1) {
		dev->keypad = KEY_BACK;
		input_report_key(dev->input, KEY_BACK, 1);
		enable_irq(dev->client->irq);
		return;
	}

	// F11_2D_Data00 (0x01)
	// Do not use, Finger State 1, Finger State 0 (0x01)
	//         0000,                 XX,                  01


   input_info = data[0x15 - 0x13];
   /* 090825_JIHYE AHN_finger status modified  */ 
   if(input_info == 0) {
		enable_irq(dev->client->irq);
		return;
	}

	xh = data[0x16 - 0x13];
	yh = data[0x17 - 0x13];
	xyl = data[0x18 - 0x13];

	x = ((xh << 4) & 0xff0) + (xyl & 0x0f);
	y = ((yh << 4) & 0xff0) + ((xyl & 0xf0) >> 4);

	x = ((SPEC_X_MAX - x) / REAL_PER_SPEC_X_RATE);
	y = ((SPEC_Y_MAX - y) / REAL_PER_SPEC_Y_RATE);
   
 //  printk("x = %d, y = %d\n",x,y);
   

   ts_update_pen_state(dev,x,y);       /* 090827_JIHYE AHN_applied algorithm for touch stability*/
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));
	enable_irq(dev->client->irq);
}

static int t1021_irq_handler(int irq, void *dev_id)
{
   struct t1021_ts *dev = (struct t1021_ts *)dev_id;

	disable_irq(irq);
	queue_work(touch_wq, &dev->work);

   return IRQ_HANDLED;
}

static int t1021_probe_chip(struct i2c_client *client)
{	
	int ret;
	int i;
         
   JIHYE_TRACE("t1021_probe_chip\n");

	msleep(200);

	for (i = 0x13; i < 0x24; ++i) {
		ret = i2c_smbus_read_byte_data(client, i);
	}
	return 0;
}

static int t1021_probe(struct i2c_client *client, const struct i2c_device_id *i2c_dev)
{
	int ret;
	struct t1021_ts *dev;
	struct input_dev *input;
	struct vreg *vreg_msmp;
	struct vreg *vreg_gp4;

   JIHYE_TRACE("t1021_probe\n");

   vreg_msmp = vreg_get(0, "msmp");
	vreg_enable(vreg_msmp);
	ret = vreg_set_level(vreg_msmp, T1021_V);
	if(ret != 0) {
		return -1;
	}
	
	vreg_gp4 = vreg_get(0, "gp4");
	vreg_enable(vreg_gp4);
	ret = vreg_set_level(vreg_gp4, T1021_V);
	if (ret != 0) {
		return -1;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
      ret = -ENODEV;
      goto err_check_functionality;
   }

	ret = t1021_probe_chip(client);
	if (ret != 0) {
		goto err_probe_chip;
	}

	dev = kzalloc(sizeof(struct t1021_ts), GFP_KERNEL);
	if (dev == 0) {
		ret = -ENOMEM;
		goto err_alloc_data;
	}

	INIT_WORK(&dev->work, t1021_work_func);
	dev->client = client;
	dev->irq = 1;

	i2c_set_clientdata(client, dev);
	
	dev->input = input_allocate_device();
	if (dev->input == 0) {
		ret = -ENOMEM;
		goto err_input_allocate_device;
	}

	input = dev->input;
	input->name = TS_DRIVER_NAME;
	input->phys = "t1021_touch/input0";
   input->id.bustype = BUS_HOST;
   input->id.vendor = 0x0001;
   input->id.product = 0x0002;
   input->id.version = 0x0100;

	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
   input->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
   input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

   input_set_abs_params(input, ABS_X, REAL_TS_X_MIN, REAL_TS_X_MAX, 0, 0);
   input_set_abs_params(input, ABS_Y, REAL_TS_Y_MIN, REAL_TS_Y_MAX, 0, 0);
   input_set_abs_params(input, ABS_PRESSURE, 0, TS_PRESSURE_MAX, 0, 0);

	set_bit(KEY_MENU, input->keybit);
	set_bit(KEY_BACK, input->keybit);	

   ret = input_register_device(input);
	if(ret) {
        goto err_input_register_device;
   }
   dev->count = 0;
         
	setup_timer(&dev->timer, ts_timer, (unsigned long)dev);
	setup_timer(&dev->btn_timer, ts_btn_timer, (unsigned long)dev);
	
   if (dev->irq && dev->client->irq) {
      ret = request_irq(dev->client->irq, t1021_irq_handler,
            IRQF_TRIGGER_FALLING,
            TS_DRIVER_NAME, dev);
      if (ret) {
         goto err_request_irq;
      }
   }
#ifdef CONFIG_HAS_EARLYSUSPEND
   dev->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1; 
   dev->early_suspend.suspend = t1021_early_suspend;
   dev->early_suspend.resume = t1021_early_resume;
   register_early_suspend(&dev->early_suspend);
#endif

   dev_info(&client->dev, "Capacitive touch(t1021) probed\n");

   return 0;

err_request_irq:
	input_unregister_device(input);
err_input_register_device:
	input_free_device(input);
err_input_allocate_device:	
err_alloc_data:
err_probe_chip:
err_check_functionality:
	
    return ret;
}

static int t1021_remove(struct i2c_client *client)
{
	struct t1021_ts *dev = i2c_get_clientdata(client);

	if (dev->irq && dev->client->irq) {
		free_irq(dev->client->irq, dev);
	}

	input_unregister_device(dev->input);
	input_free_device(dev->input);

	kfree(dev);

	return 0;
}

static struct i2c_device_id t1021_idtable[] = {
    {TS_DRIVER_NAME, 1},
};

static int t1021_suspend(struct i2c_client *client, pm_message_t mesg)
{       
   int test1=0;
   int test2=0;
   int ret;
	struct t1021_ts *dev = i2c_get_clientdata(client);

   JIHYE_TRACE("SUCCESS\n");

	if (dev->irq)
		disable_irq(client->irq);

   ret = cancel_work_sync(&dev->work);
   /* if work was pending disable-count is now 2 */
   if (ret && dev->irq) 
		enable_irq(dev->client->irq);

   ret = i2c_smbus_write_byte_data(client, 0x25, 0); /* disable interrupt */
	if (ret != 0)
		JIHYE_TRACE("synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");

   test1 = i2c_smbus_read_byte_data(client, 0x25);
   JIHYE_TRACE("0x25 = 0x%x\n",test1);

   ret++;
         
	ret = i2c_smbus_write_byte_data(client, 0x24, 0x01); /* deep sleep :    0  0  XXX  0  01 */  
	if (ret != 0)
	JIHYE_TRACE ("synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");

   test2 = i2c_smbus_read_byte_data(client, 0x24);
   JIHYE_TRACE("0x24 = 0x%x\n",test2);

   return 0;
}

static int t1021_resume(struct i2c_client *client)
{
	int ret;
	struct t1021_ts *dev = i2c_get_clientdata(client);

   JIHYE_TRACE("SUCCESS\n");

   t1021_probe_chip(client);   // synaptics_init_panel(ts);

	if (dev->irq)
		enable_irq(client->irq);

   ret =  i2c_smbus_write_byte_data(client, 0x25, 0x0f); /* enable abs int */
   if (ret != 0) {
      JIHYE_TRACE( "data write :  failed\n");
      return -1;
   }
        
   ret = i2c_smbus_write_byte_data(client, 0x24, 0); /* deep sleep :    0  0  XXX  0  01 */  
	return 0;
}

static void t1021_early_suspend(struct early_suspend *h)
{       
	struct t1021_ts *ts;

   JIHYE_TRACE("START\n");
	ts = container_of(h, struct t1021_ts, early_suspend);
	t1021_suspend(ts->client, PMSG_SUSPEND);
   JIHYE_TRACE( "END");
}

static void t1021_early_resume(struct early_suspend *h)
{      
	struct t1021_ts *ts;

   JIHYE_TRACE("SUCCESS\n");
	ts = container_of(h, struct t1021_ts, early_suspend);
	t1021_resume(ts->client);
}

static struct i2c_driver t1021_driver = {
    .probe     = t1021_probe,
    .remove    = t1021_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend   = t1021_suspend,
    .resume    = t1021_resume,
#endif
    .id_table  = t1021_idtable,
    .driver    = {
        .name  = TS_DRIVER_NAME,
    },
};

static int __devinit t1021_init(void)
{
   touch_wq = create_singlethread_workqueue("touch_wq");
   if (!touch_wq) {
      return -ENOMEM;
   }
	return i2c_add_driver(&t1021_driver);    
}

static void __exit t1021_exit(void)
{
	i2c_del_driver(&t1021_driver);
}

module_init(t1021_init);
module_exit(t1021_exit);

MODULE_DESCRIPTION("touch-screen device driver using t1021 interface");
MODULE_AUTHOR("Han Jun Yeong <j.y.han@lge.com>");
MODULE_LICENSE("GPL");

