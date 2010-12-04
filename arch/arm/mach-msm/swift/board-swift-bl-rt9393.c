
/*
 *  arch/arm/mach-msm/swift/Board-swift-bl-rt9393.c 
 * - Qualcomm MSM 7X00A backlight Driver (RT9393)
 * 
 *  Copyright (C) 2009 LGE Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Author: Martin, Cho (martin81@lge.com)
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

#include <linux/mutex.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/div64.h>

static uint32_t msm_bl_debug = 1;
module_param_named(debug_mask, msm_bl_debug, uint, 0664);

#define CONFIG_RT9393_BACKLIGHT_DEBUG

#ifdef CONFIG_RT9393_BACKLIGHT_DEBUG
#define MARTIN_TRACE(fmt, args...)  printk(KERN_ERR "MARTIN-DBG[%-18s:%5d]" fmt, __FUNCTION__, __LINE__, ## args)
#define MARTIN_ERROR(fmt, args...)  printk(KERN_ERR "MARTIN-ERR[%-18s:%5d]" fmt, __FUNCTION__, __LINE__, ## args)
#else
#define MARTIN_TRACE(fmt, args...)
#define MARTIN_ERROR(fmt, args...)
#endif

/////////////////////////////

#define GPIO_LCD_BL_EN      84                  // GPIO PIN of EN_VALUE

static int MAX = 32;                                      // Max brightness
static int intensity = 0;                                 // 1 ~ 32 ( 0 : power off)
static int pulse_count = 0;                           // 32 step pulse  
static int en_value = 0;                                // EN_VALUE : 0 or 1
static int  onoff = 0;                                      // Flag for Backlight on/off
static struct backlight_device *pbd;

static spinlock_t ops_lock = SPIN_LOCK_UNLOCKED;

int rt9393_set_envalue()
{
        gpio_set_value(GPIO_LCD_BL_EN, en_value);
        return 0;
}

int rt9393_poweron()
{
        en_value = 1;
        rt9393_set_envalue();
        udelay(50);
       
        onoff = 1;

        pulse_count = 0;

	if (msm_bl_debug & 1)
        MARTIN_TRACE("pulse_count : %d, intensity : %d \n", pulse_count, intensity);
       
        return en_value;
}

int rt9393_poweroff()
{
        en_value = 0;
        rt9393_set_envalue();
        mdelay(4);
         
        onoff = 0;
        
        intensity = 0;

        if (msm_bl_debug & 1){
        MARTIN_TRACE("Poweroff is called \n");
        MARTIN_TRACE("pulse_count : %d, intensity : %d \n", pulse_count, intensity);
        }
        return 0;
}

int rt9393_en_pulsecontrol(int t_intensity)
{
         int pulsenumber;
      
        if(intensity == t_intensity) {
            return 0;
            }
        
       if(t_intensity == 0 ||t_intensity < 0) {
          rt9393_poweroff();
          return 0;
        }
    
        if(t_intensity > 32) {
             if (msm_bl_debug & 1)
            MARTIN_TRACE("Intensity value is error.\n");
            intensity = MAX;             
        }
        else{
            intensity = t_intensity;
        }
      
        // Power on
        if(onoff == 0) {
         rt9393_poweron();
            if(intensity == 32) {
                return 0;
                }
        }
        
        do{
               pulse_count++;
               if(pulse_count == 32) {
                    pulse_count = 0;
                }

               pulsenumber = 32 - pulse_count;
 
               en_value = 0;
               rt9393_set_envalue();
               udelay(2);
        
               en_value = 1;
               rt9393_set_envalue();
               udelay(2);
                    
        }while(intensity != pulsenumber);

          if (msm_bl_debug & 1)
          MARTIN_TRACE("pulse_count : %d, intensity : %d \n", pulse_count, intensity);
          return 0;
}

int rt9393_send_intensity(struct backlight_device *bd)
{
        spin_lock_irq(&ops_lock);
        rt9393_en_pulsecontrol(bd->props.brightness);
        spin_unlock_irq(&ops_lock);    
        return 0;
}

int rt9393_get_intensity()
{
        return  intensity;
}

int rt9393_set_intensity(struct backlight_device * bd)
{
        rt9393_send_intensity(bd);
        return  intensity;
}
EXPORT_SYMBOL(rt9393_set_intensity);

static ssize_t
show_intensity(struct device *dev, struct device_attribute * attr, char *buf)
{
       if (msm_bl_debug & 1){
           MARTIN_TRACE("show_intensity is called \n");
           MARTIN_TRACE("value of intensity : %d \n", intensity);
           }
       return 0;
}

static ssize_t
change_intensity(struct device *dev, struct device_attribute * attr,
                                                                                       const char * buf, size_t count)
{
        int value;
     
        sscanf(buf, "%d", &value);
        if (msm_bl_debug & 1)
        MARTIN_TRACE("change intensity  value : %d is called \n", value);

        rt9393_en_pulsecontrol(value);
        
        if (msm_bl_debug & 1)
        MARTIN_TRACE("pulse_count : %d, intensity : %d\n", pulse_count, intensity);
        
        return 0;
}

static DEVICE_ATTR(show_intensity, 0664, show_intensity, change_intensity);

static struct backlight_ops rt9393_ops = {
//	.options = BL_CORE_SUSPENDRESUME,
	.get_brightness = rt9393_get_intensity,
	.update_status  = rt9393_set_intensity,
};

static int
rt9393_probe(struct platform_device *pdev)
{
#ifndef CONFIG_FB_BACKLIGHT
        if (msm_bl_debug & 1)
        MARTIN_TRACE("rt9393 probe is called \n");
        pbd = backlight_device_register("rt9393", NULL, NULL, &rt9393_ops);
#endif

        onoff = 0;
        en_value = 0;
        pulse_count = 0;

        pbd->props.max_brightness = MAX;
        pbd->props.power=0;
        pbd->props.brightness =MAX/2;

        rt9393_set_intensity(pbd);
        device_create_file(&pbd->dev, &dev_attr_show_intensity);

        if (msm_bl_debug & 1)
        MARTIN_TRACE("[Poweron] pulse_count : %d, intensity : %d \n",
                         pulse_count, intensity);
	return 0;
}

static int rt9393_remove(struct platform_device *pdev)
{
        backlight_device_unregister(pbd);
	return 0;
}

static struct platform_driver rt9393_backlight_driver = {
	.probe		= rt9393_probe,
	.remove		= rt9393_remove,
	.driver		= {
		.name	= "rt9393-bl",
                 .owner       = THIS_MODULE,
	},
};

static int __init rt9393_init(void)
{
        if (msm_bl_debug & 1)
        MARTIN_TRACE("[Sungwoo] dummy_init is called\n");
        
#ifndef CONFIG_FB_BACKLIGHT
       return platform_driver_register(&rt9393_backlight_driver);
#else
       return 0;
#endif        
}

static void __exit rt9393_exit(void)
{
#ifndef CONFIG_FB_BACKLIGHT
        platform_driver_unregister(&rt9393_backlight_driver);
#endif
}

module_init(rt9393_init);
module_exit(rt9393_exit);

MODULE_LICENSE("GPL");


