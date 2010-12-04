
/*
 *  linux/drivers/video/backlight/dummy_backlight.c 
 * - Qualcomm MSM 7X00A backlight  dummy Driver (RT9393)
 * 
 *  Copyright (C) 2009 LGE Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Author: Martin, Cho (martin81@lge.com)
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>

#include <asm/uaccess.h>
#include <asm/io.h>

///////////////////////
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/div64.h>

/////////////////////////////

#define GPIO_LCD_BL_EN      84

static int max = 32;

static int intensity = 0;
static int pulse_count = 0;
static int en_value = 0;

static int  onoff = 0;

static struct backlight_device *bd;

#define DUMMY_DEV_NAME "dumy_backlightdev"
// #define DUMMY_DEV_MAJOR 240

int dummy_open(struct inode *inode, struct file * filp)
{
        return 0;
}

int dummy_rt9393_set_intensity()
{
     gpio_set_value(GPIO_LCD_BL_EN, en_value);
//    printk("[Set_intensity] en_value : %d\n", en_value);
     return 0;
}

int dummy_rt9393_get_intensity()
{
     return  intensity;
}

int dummy_rt9393_poweron()
{
//       pulse_count = 0;

       en_value = 0;
        dummy_rt9393_set_intensity();
        udelay(2);
        
       en_value = 1;
       dummy_rt9393_set_intensity();
       udelay(50);
       
       onoff = 1;
        #if defined(CONFIG_BACKLIGHT_LEDS_CLASS)
	saved_pleddev->brightness = intensity;
        #endif
       
        printk("[Poweron] pulse_count : %d, intensity : %d \n",
         pulse_count, intensity);
       
        return en_value;
}

int dummy_rt9393_poweroff()
{
        en_value = 0;
        dummy_rt9393_set_intensity();
         mdelay(3);
         
        onoff = 0;
        
        #if defined(CONFIG_BACKLIGHT_LEDS_CLASS)
        saved_pleddev->brightness = 0;
        #endif	
       
        printk("[Sungwoo] Poweroff is called \n");
        
        return 0;
}

int dummy_rt9393_en_pulsecontrol(int t_intensity)
{
         int pulsenumber;

         if(t_intensity < 0) {
             printk("Intensity value is error.\n");
             return 0;
        }
         
        if(intensity == t_intensity) {
            return 0;
            }
        
        intensity = t_intensity;
        
      if(intensity == 0) {
          dummy_rt9393_poweroff();
          return 0;
     }
      
    // Power on
    if(onoff == 0) {
         dummy_rt9393_poweron();
        }
          
    do{
        pulse_count++;
        
        if(pulse_count == 32) {
            pulse_count = 0;
        }

        pulsenumber = 32 - pulse_count;

//        printk("pulse_count : %d, intensity : %d, pulsenumber : %d \n",
//         pulse_count, intensity, pulsenumber);
        
        en_value = 0;
        dummy_rt9393_set_intensity();
        udelay(2);
        
        en_value = 1;
        dummy_rt9393_set_intensity();
        udelay(2);
          
         }while(intensity != pulsenumber);

         return 0;
}

int dummy_rt9393_send_intensity(struct backlight_device *bd)
{

    intensity = bd->props.brightness;
   dummy_rt9393_en_pulsecontrol(intensity);
    
     return 0;
}

static ssize_t
show_intensity(struct device *dev, struct device_attribute * attr, char *buf)
{
       printk("[Sungwoo] show_intensity is called \n");
       printk("[Sungwoo] value of intensity : %d \n", intensity);
       return 0;
}

static ssize_t
change_intensity(struct device *dev, struct device_attribute * attr,
                                                                                       const char * buf, size_t count)
{
        int value;
     
        sscanf(buf, "%d", &value);
       
        printk("[Sungwoo] change intensity  value : %d is called \n", value);

        dummy_rt9393_en_pulsecontrol(value);

         printk("pulse_count : %d, intensity : %d\n", pulse_count, intensity);
        
         return 0;
}

static DEVICE_ATTR(show_intensity, 0664, show_intensity, change_intensity);


int dummy_release(struct inode * inode, struct file * filp)
{
        return 0;
}

struct file_operations dummy_fops =
{
        .owner = THIS_MODULE,
        .open = dummy_open,
        .release = dummy_release,
};

static struct backlight_ops dummy_rt9393_ops = {
//	.options = BL_CORE_SUSPENDRESUME,
	.get_brightness = dummy_rt9393_get_intensity,
	.update_status  = dummy_rt9393_send_intensity,
};


static int
dummy_probe(struct platform_device *pdev)
{
        printk("[Sungwoo] dummy_backlight probe is called \n");
        bd = backlight_device_register("rt9393", NULL, NULL, &dummy_rt9393_ops);

        onoff = 0;
        en_value = 0;
        pulse_count = 0;
        
        dummy_rt9393_en_pulsecontrol(max/2);
        
        device_create_file(&(pdev->dev), &dev_attr_show_intensity);
	return 0;
}


static int dummy_suspend(struct platform_device *pdev)
{
 
         intensity = 10;
//         saved_pleddev->brightness = 0;
         dummy_rt9393_en_pulsecontrol(intensity);

        mdelay(10);

          intensity = 0;
    //         saved_pleddev->brightness = 0;
             dummy_rt9393_en_pulsecontrol(intensity);

    printk("[Sungwoo] dummy_backlight suspend is called \n");
 
	return 0;
}

static void dummy_resume(void)
{
	intensity = 16;
        dummy_rt9393_en_pulsecontrol(intensity);
        
        printk("[Sungwoo] dummy_backlight resume is called \n");        
}



static int dummy_remove(struct platform_device *pdev)
{
        intensity = 0;
        en_value = 0;
        pulse_count = 0;
        
	bd->props.power = 0;
    
	bd->props.brightness = 0;

        backlight_device_unregister(bd);
	return 0;
}

static struct platform_driver dummy_backlight_driver = {
	.probe		= dummy_probe,
	.remove		= dummy_remove,
	.suspend	         = dummy_suspend,
	.resume		= dummy_resume,
	.driver		= {
		.name	= "rt9393-bl",
	},
};

int dummy_init(void)
{
        printk("[Sungwoo] dummy_init is called\n");

        return platform_driver_register(&dummy_backlight_driver);
}

void dummy_exit(void)
{
        platform_driver_unregister(&dummy_backlight_driver);
}

module_init(dummy_init);
module_exit(dummy_exit);

MODULE_LICENSE("GPL");


