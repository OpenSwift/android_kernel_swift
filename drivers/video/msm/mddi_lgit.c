/* drivers/video/msm/src/panel/mddi/mddi_innotek.c
 *
 * Copyright (C) 2008 QUALCOMM Incorporated.
 * Copyright (c) 2008 QUALCOMM USA, INC.
 * 
 * All source code in this file is licensed under the following license
 * except where indicated.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#include "msm_fb.h"
#include "mddihost_eve.h"
#include "mddihosti.h"
#include <asm/gpio.h>
#include <mach/vreg.h>


#define PANEL_DEBUG 0

static uint32 mddi_innotek_curr_vpos;
static boolean mddi_innotek_monitor_refresh_value = FALSE;
static boolean mddi_innotek_report_refresh_measurements = FALSE;
static boolean is_lcd_on = FALSE;

/* The comment from AMSS codes:
 * Dot clock (10MHz) / pixels per row (320) = rows_per_second
 * Rows Per second, this number arrived upon empirically 
 * after observing the timing of Vsync pulses
 * XXX: TODO: change this values for INNOTEK PANEL */
static uint32 mddi_innotek_rows_per_second = 31250;
static uint32 mddi_innotek_rows_per_refresh = 480;
static uint32 mddi_innotek_usecs_per_refresh = 15360; /* rows_per_refresh / rows_per_second */
extern boolean mddi_vsync_detect_enabled;

static msm_fb_vsync_handler_type mddi_innotek_vsync_handler = NULL;
static void *mddi_innotek_vsync_handler_arg;
static uint16 mddi_innotek_vsync_attempts;

static struct msm_panel_common_pdata *mddi_innotek_pdata;

static int mddi_innotek_lcd_on(struct platform_device *pdev);
static int mddi_innotek_lcd_off(struct platform_device *pdev);

static int mddi_innotek_lcd_init(void);
static void mddi_innotek_lcd_panel_poweron(void);
static void mddi_innotek_lcd_ff_80(void);
static void mddi_innotek_lcd_ff_60(void);
void mddi_innotek_lcd_ff_change(int report_value);



static int state_suspend = 0;


#define DEBUG 0
#if DEBUG
#define EPRINTK(fmt, args...) printk(fmt, ##args)
#else
#define EPRINTK(fmt, args...) do { } while (0)
#endif

struct display_table {
    unsigned reg;
    unsigned char count;
    unsigned char val_list[15];
};

#define REGFLAG_DELAY             0XFFFE
#define REGFLAG_END_OF_TABLE      0xFFFF   // END OF REGISTERS MARKER

#define GPIO_LCD_BL_EN	        34
#define GPIO_LCD_RESET_N        102

#define GPIO_HIGH_VALUE 1
#define GPIO_LOW_VALUE  0

static struct display_table mddi_innotek_display_off_data[] = {

	// Display off sequence
	{0xEF, 1, {0x06}},
	{REGFLAG_DELAY, 20, {}},
	{0xEF, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_sleep_mode_on_data[] = {

	// Sleep mode on
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_sleep_mode_off_data[] = {

	// Sleep mode off
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x2C, 1, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_display_on_data[] = {

	/* Power ON Sequence */
    {0xF3, 1, {0x00}},
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},
    {0xF3, 9, {0x00, 0x01, 0x00, 0x00, 0x07, 0x44, 0x32, 0x7F, 0x00}},
    {0xF4, 5, {0x19, 0x23, 0x31, 0x55, 0x11}},
    {0xF5, 6, {0x10, 0x00, 0x08, 0xF0, 0x35, 0x1F}},
    {REGFLAG_DELAY, 10, {}},
    {0xF3, 2, {0x00, 0x03}},    
    {REGFLAG_DELAY, 10, {}},
    {0xF3, 2, {0x00, 0x07}},    
    {REGFLAG_DELAY, 10, {}},
    {0xF3, 2, {0x00, 0x0F}},    
    {REGFLAG_DELAY, 10, {}},
    {0xF3, 2, {0x00, 0x1F}},    
    {REGFLAG_DELAY, 10, {}},
    {0xF3, 2, {0x00, 0x3F}},    
    {REGFLAG_DELAY, 20, {}},
    {0xF3, 2, {0x00, 0x7F}},
    {REGFLAG_DELAY, 30, {}},

    // Normal Mode
    {0x13, 1, {0x00}},

    // Column Address
    {0x2A, 4, {0x00, 0x00, 0x01, 0x3F}},

    // Page Address
    {0x2B, 4, {0x00, 0x00, 0x01, 0xDF}},

    // Memory Data Address Control
    {0x36, 1, {0x48}},

    // Interface Pixel Format
    {0x3A, 1, {0x05}},

	// Display Control Set
    {0xF2, 11, {0x11, 0x14, 0x03, 0x03, 0x03, 0x08, 0x08, 0x03, 0x00, 0x15, 0x15}},

	// Tearing Effect Lion On
    {0x35, 1, {0x00}},

	// CABC Control
    {0x51, 1, {0xFF}},
    {0x53, 1, {0x2C}},
    {0x55, 1, {0x03}},
    {0x5E, 1, {0x00}},
    {0xCA, 3, {0x80, 0x80, 0x3F}},
    {0xCB, 1, {0x03}},
    {0xCC, 5, {0x20, 0x01, 0x8F, 0x00, 0xEF}},
    {0xCD, 2, {0x04, 0x97}},

    // Positive Gamma Red
    {0xF7, 15, {0x00, 0x0D, 0x06, 0x17, 0x1A, 0x29, 0x23, 0x24, 0x1A, 0x1F, 0x12, 0x15, 0x16, 0x00, 0x00}},

    // Negative Gamma Red
    {0xF8, 15, {0x00, 0x0D, 0x06, 0x17, 0x1A, 0x29, 0x23, 0x24, 0x1A, 0x1F, 0x12, 0x15, 0x16, 0x00, 0x00}},

    // Positive Gamma Green
    {0xF9, 15, {0x00, 0x0D, 0x06, 0x17, 0x1A, 0x29, 0x23, 0x24, 0x1A, 0x1F, 0x12, 0x15, 0x16, 0x00, 0x00}},

    // Negative Gamma Green
    {0xFA, 15, {0x00, 0x0D, 0x06, 0x17, 0x1A, 0x29, 0x23, 0x24, 0x1A, 0x1F, 0x12, 0x15, 0x16, 0x00, 0x00}},

    // Positive Gamma Blue
    {0xFB, 15, {0x00, 0x0D, 0x06, 0x17, 0x1A, 0x29, 0x23, 0x24, 0x1A, 0x1F, 0x12, 0x15, 0x16, 0x00, 0x00}},

    // Negative Gamma Blue
    {0xFC, 15, {0x00, 0x0D, 0x06, 0x17, 0x1A, 0x29, 0x23, 0x24, 0x1A, 0x1F, 0x12, 0x15, 0x16, 0x00, 0x00}},

    // Gate Control Register
    {0xFD, 3, {0x11, 0x3B, 0x00}},

    // GRAM Write
    {0x2C, 1, {0x00}},

    // Dispaly On Sequence
    {0xEF, 1, {0x06}},
    {REGFLAG_DELAY, 20, {}},
    {0xEF, 1, {0x07}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_call_mode_on_data[] = {

	// Normal mode on
	{0x13, 1, {0x00}},

	// Change the Frame Frequency
	{0xF2, 11, {0x13, 0x13, 0x03, 0x08, 0x08, 0x08, 0x08, 0x10, 0x00, 0x13, 0x13}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct display_table mddi_innotek_call_mode_off_data[] = {

	// Normal mode on
	{0x13, 1, {0x00}},

	// Change the Frame Frequency
	{0xF2, 11, {0x16, 0x16, 0x03, 0x08, 0x08, 0x08, 0x08, 0x10, 0x00, 0x16, 0x16}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void display_table(struct display_table *table, unsigned int count)
{

	unsigned int i;

	EPRINTK("%s: started. count : %d\n", __func__, count);

#if PANEL_DEBUG
    unsigned int j;
    for (i = 0; i < count; i++) {
        printk("[%d] =============================\n", i);

        if (table[i].reg == REGFLAG_DELAY) {
            printk("=================================\n");
            continue;
        }

        for (j = 0; j < table[i].count; j++) {
            printk("%d: 0x%x\n", j, table[i].val_list[j]);
        }
        printk("=================================\n");
    }
#endif /* PANEL_DEBUG */

    for(i = 0; i < count; i++) {
        unsigned reg;

        reg = table[i].reg;
        switch (reg) {
            case REGFLAG_DELAY:
#if PANEL_DEBUG
                printk("delay: %d\n", table[i].count);
#endif
                mdelay(table[i].count);
                break;
            case REGFLAG_END_OF_TABLE:
#if PANEL_DEBUG
                printk("%s: end of the table\n", __func__);
#endif
                break;
            default:
                mddi_host_register_cmd_write(
                        reg,
                        table[i].count,
                        table[i].val_list,
                        0,
                        0,
                        0);
				EPRINTK("%s: reg : %x, val : %x.\n", __func__, reg, table[i].val_list[0]);
        	}
    	}
	
}


static void mddi_innotek_vsync_set_handler(msm_fb_vsync_handler_type handler,	/* ISR to be executed */
					 void *arg)
{
	boolean error = FALSE;
	unsigned long flags;

	/* Disable interrupts */
	spin_lock_irqsave(&mddi_host_spin_lock, flags);
	// INTLOCK();

	if (mddi_innotek_vsync_handler != NULL) {
		error = TRUE;
	} else {
		/* Register the handler for this particular GROUP interrupt source */
		mddi_innotek_vsync_handler = handler;
		mddi_innotek_vsync_handler_arg = arg;
	}

	/* Restore interrupts */
	spin_unlock_irqrestore(&mddi_host_spin_lock, flags);
	// MDDI_INTFREE();
	if (error) {
		MDDI_MSG_ERR("MDDI: Previous Vsync handler never called\n");
		printk("MDDI: Previous Vsync handler never called\n");
	} else {
		/* TODO: Enable the vsync wakeup */

		mddi_innotek_vsync_attempts = 1;
		mddi_vsync_detect_enabled = TRUE;
	}
}

static void mddi_innotek_lcd_vsync_detected(boolean detected)
{
	// static timetick_type start_time = 0;
	static struct timeval start_time;
	static boolean first_time = TRUE;
	// uint32 mdp_cnt_val = 0;
	// timetick_type elapsed_us;
	struct timeval now;
	uint32 elapsed_us;
	uint32 num_vsyncs;

	if ((detected) || (mddi_innotek_vsync_attempts > 5)) {
		if ((detected) && (mddi_innotek_monitor_refresh_value)) {
			// if (start_time != 0)
			if (!first_time) {
				// elapsed_us = timetick_get_elapsed(start_time, T_USEC);
				jiffies_to_timeval(jiffies, &now);
				elapsed_us =
				    (now.tv_sec - start_time.tv_sec) * 1000000 +
				    now.tv_usec - start_time.tv_usec;
				/* LCD is configured for a refresh every * usecs, so to
				 * determine the number of vsyncs that have occurred
				 * since the last measurement add half that to the
				 * time difference and divide by the refresh rate. */
				num_vsyncs = (elapsed_us +
					      (mddi_innotek_usecs_per_refresh >>
					       1)) /
				    mddi_innotek_usecs_per_refresh;
				/* LCD is configured for * hsyncs (rows) per refresh cycle.
				 * Calculate new rows_per_second value based upon these
				 * new measurements. MDP can update with this new value. */
				mddi_innotek_rows_per_second =
				    (mddi_innotek_rows_per_refresh * 1000 *
				     num_vsyncs) / (elapsed_us / 1000);
			}
			// start_time = timetick_get();
			first_time = FALSE;
			jiffies_to_timeval(jiffies, &start_time);
			if (mddi_innotek_report_refresh_measurements) {
			}
		}
		/* if detected = TRUE, client initiated wakeup was detected */
		if (mddi_innotek_vsync_handler != NULL) {
			(*mddi_innotek_vsync_handler)
			    (mddi_innotek_vsync_handler_arg);
			mddi_innotek_vsync_handler = NULL;
		}
		mddi_vsync_detect_enabled = FALSE;
		mddi_innotek_vsync_attempts = 0;
		/* need to disable the interrupt wakeup */
		/* TODO: how to? */
		if (!detected) {
			/* give up after 5 failed attempts but show error */
			MDDI_MSG_NOTICE("Vsync detection failed!\n");
			printk("Vsync detection failed!\n");
		} else if ((mddi_innotek_monitor_refresh_value) &&
			   (mddi_innotek_report_refresh_measurements)) {
			MDDI_MSG_NOTICE("  Last Line Counter=%d!\n",
					mddi_innotek_curr_vpos);
			printk("  Last Line Counter=%d!\n",
				mddi_innotek_curr_vpos);
			// MDDI_MSG_NOTICE("  MDP Line Counter=%d!\n",mdp_cnt_val);
			MDDI_MSG_NOTICE("  Lines Per Second=%d!\n",
					mddi_innotek_rows_per_second);
			printk("  Lines Per Second=%d!\n",
				mddi_innotek_rows_per_second);
		}
		/* clear the interrupt */
		/* XXX: we do not have to */
	} else {
		/* if detected = FALSE, we woke up from hibernation, but did not
		 * detect client initiated wakeup.
		 */
		mddi_innotek_vsync_attempts++;
	}
}

static int mddi_innotek_lcd_on(struct platform_device *pdev)
{
	EPRINTK("%s: started.\n", __func__);

	if(state_suspend == 1) 
	{
		mdelay(1);		
		
		mddi_innotek_lcd_panel_poweron();

		mdelay(5);

		
		display_table(mddi_innotek_sleep_mode_off_data,
								sizeof(mddi_innotek_sleep_mode_off_data) / sizeof(struct display_table));
		state_suspend = 0;
	}
	else {
	// LCD HW Reset
	mddi_innotek_lcd_panel_poweron();
	
	/* Set the MDP pixel data attributes for Primary Display */
	// mddi_host_write_pix_attr_reg(0x00C3);

	display_table(mddi_innotek_display_on_data,
					sizeof(mddi_innotek_display_on_data) / sizeof(struct display_table));
	}

	is_lcd_on = TRUE;
	return 0;
}

static int mddi_innotek_lcd_off(struct platform_device *pdev)
{
	EPRINTK("%s: started.\n", __func__);

	is_lcd_on = FALSE;
	gpio_set_value(GPIO_LCD_RESET_N, 0);
	mdelay(120);

	
	display_table(mddi_innotek_display_off_data,
					sizeof(mddi_innotek_display_off_data) / sizeof(struct display_table));
	display_table(mddi_innotek_sleep_mode_on_data,
					sizeof(mddi_innotek_sleep_mode_on_data) / sizeof(struct display_table));
	
	return 0;
}

struct msm_fb_panel_data innotek_panel_data0 = {
	.on = mddi_innotek_lcd_on,
	.off = mddi_innotek_lcd_off,
	.set_backlight = NULL,
	.set_vsync_notifier = mddi_innotek_vsync_set_handler,
};

static struct platform_device this_device_0 = {
	.name   = "mddi_innotek_hvga",
	.id	= MDDI_LCD_INNOTEK_IM300WBN1A,
	.dev	= {
		.platform_data = &innotek_panel_data0,
	}
};


static int __init mddi_innotek_lcd_probe(struct platform_device *pdev)
{
	EPRINTK("%s: started.\n", __func__);

	if (pdev->id == 0) {
		mddi_innotek_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mddi_innotek_lcd_probe,
	.driver = {
		.name   = "mddi_innotek_hvga",
	},
};

static int mddi_innotek_lcd_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MDDI_AUTO_DETECT
	u32 id;
	id = mddi_get_client_id();

	/* TODO: Check client id */

#endif

	ret = platform_driver_register(&this_driver);
	if (!ret) {
		pinfo = &innotek_panel_data0.panel_info;
		EPRINTK("%s: setting up panel info.\n", __func__);
		pinfo->xres = 320;
		pinfo->yres = 480;
		pinfo->type = MDDI_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->mddi.vdopkt = MDDI_DEFAULT_PRIM_PIX_ATTR;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 16;
	
		// vsync config
		pinfo->lcd.vsync_enable = FALSE;
		pinfo->lcd.refx100 = 
                        (mddi_innotek_rows_per_second * 100) /
                        mddi_innotek_rows_per_refresh;
		pinfo->lcd.v_back_porch = 14;
		pinfo->lcd.v_front_porch = 6;
		pinfo->lcd.v_pulse_width = 4;
		pinfo->lcd.hw_vsync_mode = FALSE;
		pinfo->lcd.vsync_notifier_period = (1 * HZ);

		pinfo->bl_max = 4;
		pinfo->bl_min = 1;
		pinfo->clk_rate = 122880000;
		pinfo->clk_min = 122800000;
		pinfo->clk_max = 130000000;
		pinfo->fb_num = 2;

		ret = platform_device_register(&this_device_0);
		if (ret)
			platform_driver_unregister(&this_driver);
	}

	if(!ret) {
		mddi_lcd.vsync_detected = mddi_innotek_lcd_vsync_detected;
	}

	return ret;
}

extern unsigned fb_width;
extern unsigned fb_height;

static void mddi_innotek_lcd_panel_poweron(void)
{
	EPRINTK("%s: started.\n", __func__);

	fb_width = 320;
	fb_height = 480;

	udelay(15);
	
	gpio_set_value(GPIO_LCD_RESET_N, 1);
	mdelay(10);

	gpio_set_value(GPIO_LCD_RESET_N, 0);
	mdelay(10);

	gpio_set_value(GPIO_LCD_RESET_N, 1);
	mdelay(10);
}

static void mddi_innotek_lcd_ff_80(void)
{
	EPRINTK("%s: started.\n", __FUNCTION__);

	if (is_lcd_on)
	{
		display_table(mddi_innotek_call_mode_on_data,
			sizeof(mddi_innotek_call_mode_on_data) / sizeof(struct display_table));
	}
}

static void mddi_innotek_lcd_ff_60(void)
{
	EPRINTK("%s: started.\n", __FUNCTION__);

	if (is_lcd_on)
	{
		display_table(mddi_innotek_call_mode_off_data,
			sizeof(mddi_innotek_call_mode_off_data) / sizeof(struct display_table));
	}
}

void mddi_innotek_lcd_ff_change(int report_value)
{
	EPRINTK("%s: started.\n", __FUNCTION__);

	if(report_value == 0)
		mddi_innotek_lcd_ff_80();
	else
		mddi_innotek_lcd_ff_60();
}
EXPORT_SYMBOL(mddi_innotek_lcd_ff_change);
module_init(mddi_innotek_lcd_init);
