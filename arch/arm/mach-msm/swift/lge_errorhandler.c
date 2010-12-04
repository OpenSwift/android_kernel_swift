/****************************************************************************
 * Created by [bluerti@lge.com]
 * 2009-07-06
 * Made this file for implementing LGE Error Hanlder 
 * *************************************************************************/
#include "lge_errorhandler.h"

#include <mach/msm_smd.h>
#include <mach/msm_iomap.h>
#include <mach/system.h>
#include <linux/io.h>
#include <linux/syscalls.h>

#include "../smd_private.h"
#include "../proc_comm.h"
#include "../modem_notifier.h"

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>

int LG_ErrorHandler_enable = 0;
static int user_keypress =0;

 int LGE_ErrorHandler_Main( int crash_side, char * message)
 {
 	

	 char * kmem_buf;
	LG_ErrorHandler_enable = 0;
	raw_local_irq_enable();

	kmem_buf = kmalloc(LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN, GFP_ATOMIC);
	memcpy(kmem_buf, message, LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN);
	switch(crash_side) {
		case MODEM_CRASH:
			display_info_LCD(crash_side, message);
			break;

		case APPL_CRASH:
			display_info_LCD(crash_side, message);
			break;


	}
	kfree(kmem_buf);
	
	raw_local_irq_disable();
	preempt_disable();
	mdelay(100);
 
	while(1)
	{
		// 1. Check Volume UP Key 

		gpio_direction_output(34,1);
		gpio_direction_output(37,0);	//volume down
		gpio_direction_output(38,1);	//volume up
		gpio_direction_input(34);
		if(gpio_get_value(34)==0){
			printk("volup\n");
			return SMSM_SYSTEM_REBOOT; //volume up key is pressed
		}
		mdelay(100);
		// 2. Check Volume DOWN Key 
		gpio_direction_output(34,1);
		gpio_direction_output(37,1);	//volume down
		gpio_direction_output(38,0);
		gpio_direction_input(34);
		if(gpio_get_value(34)==0){
			printk("voldown\n");
			return SMSM_SYSTEM_REBOOT; //volume down key is pressed
		
		}
		mdelay(100);
	}

	
 }

int display_info_LCD( int crash_side, char * message)
{
	unsigned short * buffer;
	
	buffer = kmalloc(LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN*sizeof(short), GFP_ATOMIC);


	if(buffer)
		expand_char_to_shrt(message,buffer);
	else
		printk("Memory Alloc failed!!\n");


	display_errorinfo_byLGE(crash_side, buffer,LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN );

	kfree(buffer);

	return 0;
}

void expand_char_to_shrt(char * message,unsigned short *buffer)
{
	char * src = message;
	unsigned char  * dst = (unsigned char *)buffer;
	int i=0;


	for(i=0;i<LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN; i++) {
		*dst++ = *src++;
		*dst++ = 0x0;
	}
	
}

