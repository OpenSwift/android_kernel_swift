/* drivers/input/misc/mma7660fc_motion_sensor.c
 *
 * Tri-axial acceleration sensor(mma7660fc) driver
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
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/vreg.h>      /* set a vreg */

#ifdef CONFIG_ANDROID_POWER
#include <linux/android_power.h>
#endif

#define DEBUG						0

#define MMA7660FC_IOC_MAGIC			'B'
#define MMA7660FC_READ_ACCEL_XYZ	_IOWR(MMA7660FC_IOC_MAGIC, 46, short)
#define MMA7660FC_SET_MODE			_IOWR(MMA7660FC_IOC_MAGIC, 6, unsigned char)

#define MMA7660FC_MODE_STANDBY		0xA0
#define MMA7660FC_MODE_ACTIVE		0xA1

#define USE_IRQ                		0
#define DEF_SAMPLE_RATE		    	20000000 /* 20 ms */

/* Miscellaneous device */
#define MMA7660FC_MINOR				4
#define MMA7660FC_DEV_NAME			"mma7660fc"

#define ACCEL_THRES             	1

static struct workqueue_struct *accelerometer_wq;

struct mma7660fc_device {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
	int coordinate[3];
	int thres;
	int use_irq;
	int enabled;
	int sample_rate;
#ifdef CONFIG_ANDROID_POWER
	android_early_suspend_t early_suspend;
#endif	
};

#ifdef CONFIG_ANDROID_POWER
static atomic_t suspend_flag = ATOMIC_INIT(0);
#endif

static void *mma7660fc_dev = 0;

static inline int get_acc_xyz(struct i2c_client *client, u8 command)
{
	int ret;
	u8 v;
	int output;

    /* If the Alert bit is set, the register was read at the same time
     * as the device was attempting to update the contents.
     * The register must be read again.
     */
    do{
        /*This is x or y or z -axis to read*/
	    output = i2c_smbus_read_word_data(client, command);
	    if(output < 0) {	
	        return -EIO;
	    }
    } while(output & 0x40);

    /* XOUT[5]+ XOUT[4]+ XOUT[3]+ XOUT[2]+ XOUT[1]+ XOUT[0] */
	v = output & 0x3f;

    /* XOUT[5] is sign bit (-32 <= value <= 31) */
    if((v & 0x20) == 0x20) {        
        ret = (int)((~v & 0x1f) + 0x01) * -1;
    } else {
        ret = v;
    }
 
	return ret;
}

static void mma7660fc_work_func(struct work_struct *work)
{
	struct mma7660fc_device *dev = container_of(work, struct mma7660fc_device, work);
	int i;
	int c[3];
	int d;
	int do_report = 0;
	
    /* xout : 0x00, yout: 0x01, zout: 0x02 */
	u8 v[] = {0x00, 0x01, 0x02};

    for(i = 0; i < 3; ++i) {
        /* read x, y, z -axis output */
        c[i] = get_acc_xyz(dev->client, v[i]);
        d = dev->coordinate[i] - c[i];

        if(d < 0) {
            d = -d;
        }
        		
        if(d >= dev->thres) {
	        do_report = 1;
        }
	}
	
    if(do_report) {
        for (i = 0; i < 3; ++i) {			
            if(i == 0) {    /*x*/
                input_report_abs(dev->input_dev, ABS_X + i, c[i]);
            }
            if(i == 1) {    /*y*/
                input_report_abs(dev->input_dev, ABS_Y + i, c[i]);
            }
            if(i == 2) {    /*z*/
                input_report_abs(dev->input_dev, ABS_Z + i, c[i]);
            }
        }
        input_sync(dev->input_dev);
		
        for(i = 0; i < 3; i++) {
            dev->coordinate[i] = c[i];
        }
	}

    if(dev->use_irq) {
        enable_irq(dev->client->irq);
    }
}

static void mma7660fc_get_xyz(struct mma7660fc_device* dev, short* data)
{
	int i;
	int c[3];
	
	/* xout : 0x00, yout: 0x01, zout: 0x02 */
	u8 v[] = {0x00, 0x01, 0x02};

    for(i = 0; i < 3; ++i) {
        /* read x, y, z -axis output */
        c[i] = get_acc_xyz(dev->client, v[i]);
		
		if(i == 0) {    /*x*/
            data[i] = c[i];
        }
        if(i == 1) {    /*y*/
            data[i] = c[i];
        }
        if(i == 2) {    /*z*/
            data[i] = c[i];
        }
		
		#if DEBUG
		printk("%s: data[%d] = %d\n", __FUNCTION__, i, data[i]);
		#endif
	}
}

static int mma7660fc_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int mma7660fc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int mma7660fc_ioctl(struct inode *inode, struct file *filp, 
		unsigned int cmd, unsigned long arg)
{	
	unsigned char data[6];
	struct mma7660fc_device *dev = (struct mma7660fc_device *)mma7660fc_dev;	
    if(dev == 0) {
		return -ENODEV;
    }

	switch(cmd) {
        case MMA7660FC_SET_MODE:
			if(copy_from_user((unsigned short*)data, (unsigned short*)arg, 4) != 0) {
				#if DEBUG
				printk("copy_from_user error\n");
				#endif
				return -EFAULT;
			}
			#if DEBUG
			printk("%s: cmd = MMA7660FC_SET_MODE, data = 0x%x\n", __FUNCTION__, data[0]);
			#endif
			i2c_smbus_write_byte_data(dev->client, 0x07, data[0]);
			break;
		case MMA7660FC_READ_ACCEL_XYZ:
			#if DEBUG
			printk("%s: cmd = MMA7660FC_READ_ACCEL_XYZ\n", __FUNCTION__);
			#endif
#ifdef CONFIG_ANDROID_POWER		
			if (atomic_read(&suspend_flag)) {
				memcpy(data, 0x0, 6);
			} else {
#endif			
				mma7660fc_get_xyz(dev, (short *)data);
#ifdef CONFIG_ANDROID_POWER
			}
#endif
					
			if(copy_to_user((unsigned short *)arg, (unsigned short *)data, 6) != 0)
			{
				#if DEBUG
				printk("copy_to_user error\n");
				#endif
				return -EFAULT;
			}
			break;
        default:
            return -ENOTTY;
	}

	return 0;
}

/* use miscdevice for ioctls */
static struct file_operations fops = {
    .owner      = THIS_MODULE,
    .open       = mma7660fc_open,
    .release    = mma7660fc_release,
    .ioctl      = mma7660fc_ioctl,
};

static struct miscdevice misc_dev = {
    .minor      = MMA7660FC_MINOR,
    .name       = MMA7660FC_DEV_NAME,
    .fops       = &fops,
};

static int mma7660fc_init_chip(struct i2c_client *client)
{
    int ret;
    u8 v;

    // $00, $01, $02    : X, Y, Z output value register
    // $03              : Tilt register         : (Read only)
    // $04              : Sampling Rate Status  : (Read only)
    // $05 		        : Sleep Count Register  : (Read/Write)
    // $06 		        : Interrupt Setup 
    // $07              : Mode register         

    // $06              : Interrupt Setup
    // : SHINTX SHINTY SHINTZ GINT ASINT PDINT PLINT FBINT
    // : 1          1          1           0      1        1         1       1       (0xff)
    v = 0xef & 0xff;
    ret = i2c_smbus_write_byte_data(client, 0x06, v);
	if(ret < 0) {
		printk("%s: i2c write error!\n", __FUNCTION__);
	}

    // $07            : Mode register         
    //                  : IAH IPP SCPS ASE AWE TON X MODE
    //                  : 1     1    1       1     0      0     0     1    (0xf1)
    v = 0xf1 & 0xff;
    ret = i2c_smbus_write_byte_data(client, 0x07, v);
	if(ret < 0) {
		printk("%s: i2c write error!\n", __FUNCTION__);
	}

    // $08            : Auto-Wake and Active Mode Portrait/Landscape
    //                  : FILT AWSR AMSR 
    //                  : 001  10   100
    v = 0x34 & 0xff;
    ret =  i2c_smbus_write_byte_data(client, 0x08, v);
	if(ret < 0) {
		printk("%s: i2c write error!\n", __FUNCTION__);
	}

    // $09            : Pluse Detection
    //                  : ZDA YDA XDA PDTH  (0xe0)
    //                  :  1     1     1     00000 disable pluse detection
    v = 0xe0 & 0xff;
    ret = i2c_smbus_write_byte_data(client, 0x09, v);
	if(ret < 0) {
		printk("%s: i2c write error!\n", __FUNCTION__);
	}

    return 0;
}

static int mma7660fc_probe_chip(struct i2c_client *client)
{
    int ret;

    ret = mma7660fc_init_chip(client);

    return ret;
}

static int mma7660fc_irq_handler(int irq, void *dev_id)
{
    struct mma7660fc_device *dev = dev_id;
    disable_irq(dev->client->irq);
    queue_work(accelerometer_wq, &dev->work);
    return IRQ_HANDLED;
}

#ifdef CONFIG_ANDROID_POWER
static void mma7660fc_early_suspend(android_early_suspend_t *handler)
{
#if DEBUG
	printk(KERN_INFO "%s\n", __FUNCTION__);
#endif
	atomic_set(&suspend_flag, 1);

    // $07            : Mode register         
    //                  : IAH IPP SCPS ASE AWE TON X MODE
    //                  : 1     1    1       1     0      0     0     0    (0xf0)
    v = 0xf0 & 0xff;
    ret = i2c_smbus_write_byte_data(client, 0x07, v);
	if(ret < 0) {
		printk("%s: i2c write error!\n", __FUNCTION__);
	}	
}

static void mma7660fc_early_resume(android_early_suspend_t *handler)
{
#if DEBUG
	printk(KERN_INFO "%s\n", __FUNCTION__);
#endif
    // $07            : Mode register         
    //                  : IAH IPP SCPS ASE AWE TON X MODE
    //                  : 1     1    1       1     0      0     0     1    (0xf1)
    v = 0xf1 & 0xff;
    ret = i2c_smbus_write_byte_data(client, 0x07, v);
	if(ret < 0) {
		printk("%s: i2c write error!\n", __FUNCTION__);
	}

	atomic_set(&suspend_flag, 0);	
}
#endif

static int mma7660fc_probe(struct i2c_client *client, const struct i2c_device_id *i2c_dev)
{
	int ret;
	struct mma7660fc_device *dev;
	struct input_dev *input_dev;
	struct vreg *vreg_msmp;

    vreg_msmp = vreg_get(0, "msmp");
    vreg_enable(vreg_msmp);
    ret = vreg_set_level(vreg_msmp, 2600);
    if(ret != 0) {
        return -1;
    }
	
    if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        ret = -ENODEV;
        goto err_check_functionality;
    }

    ret = mma7660fc_probe_chip(client);
    if(ret) {
        goto err_probe_chip;
    }

    dev = kzalloc(sizeof(struct mma7660fc_device), GFP_KERNEL);
    if (NULL == dev) {
        ret = -ENOMEM;
        goto err_alloc_data;
    }

    mma7660fc_dev = dev; /* for miscdevice */
    INIT_WORK(&dev->work, mma7660fc_work_func);
    dev->client         = client;
    dev->use_irq        = USE_IRQ;
    dev->thres          = ACCEL_THRES;
    dev->sample_rate    = DEF_SAMPLE_RATE;
    dev->enabled        = 1;
    i2c_set_clientdata(client, dev);

    ret = misc_register(&misc_dev);
    if(ret) {        
        goto err_miscdevice;
    }

    dev->input_dev = input_allocate_device();
    if (NULL == dev->input_dev) {
        ret = -ENOMEM;        
        goto err_input_allocate_device;
    }
    input_dev = dev->input_dev;
    input_dev->name = MMA7660FC_DEV_NAME;
    set_bit(EV_SYN, input_dev->evbit);
    set_bit(EV_ABS, input_dev->evbit);

	/* set the abs value using input_set_abs_params() */
    input_set_abs_params(input_dev, ABS_X, -32, 31, 0, 0);
    input_set_abs_params(input_dev, ABS_Y, -32, 31, 0, 0);
    input_set_abs_params(input_dev, ABS_Z, -32, 31, 0, 0);

	/*yaw*/
	input_set_abs_params(input_dev, ABS_RX, -90, 90, 0, 0);
	/*pitch*/
	input_set_abs_params(input_dev, ABS_RY, -90, 90, 0, 0);
	/*roll*/
	input_set_abs_params(input_dev, ABS_RZ, -90, 90, 0, 0); 
	
	input_set_abs_params(input_dev, ABS_WHEEL, 0, 255, 0, 0);

    ret = input_register_device(input_dev);
    if(ret) {        
        goto err_input_register_device;
    }
	
    if(dev->use_irq && dev->client->irq) {
        ret = request_irq(dev->client->irq, mma7660fc_irq_handler,
            IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, MMA7660FC_DEV_NAME, dev);
        if(ret) {
            goto err_request_irq;
        }
    }

#ifdef CONFIG_ANDROID_POWER
	dev->early_suspend.suspend = mma7660fc_early_suspend;
	dev->early_suspend.resume = mma7660fc_early_resume;
	android_register_early_suspend(&dev->early_suspend);
#endif

    dev_info(&client->dev, "motion sensor(MMA7660FC) probed\n");
    return 0;

err_request_irq:
    input_unregister_device(input_dev);
err_input_register_device:
    input_free_device(input_dev);
err_input_allocate_device:
    misc_deregister(&misc_dev);
err_miscdevice:
    kfree(dev);
err_alloc_data:
err_probe_chip:
err_check_functionality:
    return ret;
}

static int mma7660fc_remove(struct i2c_client *client)
{
	struct mma7660fc_device *dev = i2c_get_clientdata(client);

	cancel_work_sync(&dev->work);

	if (dev->use_irq && dev->client->irq) {
		free_irq(dev->client->irq, dev);
	}

	input_unregister_device(dev->input_dev);
	input_free_device(dev->input_dev);
	misc_deregister(&misc_dev);
	kfree(dev);
	return 0;
}

static struct i2c_device_id mma7660fc_idtable[] = {
    {MMA7660FC_DEV_NAME, 1},
};

static struct i2c_driver mma7660fc_driver = {
    .probe      = mma7660fc_probe,
    .remove     = mma7660fc_remove,
    .id_table   = mma7660fc_idtable,
    .driver     = {
        .name   = MMA7660FC_DEV_NAME,
    },
};

static int __devinit mma7660fc_init(void)
{
    int ret;
    
    accelerometer_wq = create_singlethread_workqueue("accelerometer_wq");
    if (!accelerometer_wq) {
        return -ENOMEM;
    }
    ret = i2c_add_driver(&mma7660fc_driver);
    
    return ret;
}

static void __exit mma7660fc_exit(void)
{
	i2c_del_driver(&mma7660fc_driver);
	
    if (accelerometer_wq) {
		destroy_workqueue(accelerometer_wq);
    }
}

module_init(mma7660fc_init);
module_exit(mma7660fc_exit);

MODULE_DESCRIPTION("3-Axis Orientation/Motion Detection Sensor");
MODULE_AUTHOR("Han Jun Yeong <j.y.han@lge.com>");
MODULE_LICENSE("GPL");

