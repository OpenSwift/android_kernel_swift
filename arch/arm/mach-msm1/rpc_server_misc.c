/* Created by princlee@lge.com  
 * arch/arm/mach-msm/rpc_server_misc.c
 *
 * Copyright (C) 2008 LGE, Inc.
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
#include <mach/msm_rpcrouter.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/eve_at.h>
#include <linux/at_kpd_eve.h>
#include <linux/input.h>
/* Misc server definitions. */

#include <asm/uaccess.h>

#define MISC_APPS_APISPROG		0x30000006
#define MISC_APPS_APISVERS		0




#define ONCRPC_MISC_APPS_APIS_NULL_PROC 0
#define ONCRPC_LCD_MESSAGE_PROC 1
#define ONCRPC_LCD_DEBUG_MESSAGE_PROC 2


#define ONCRPC_LGE_ATCMD_FACTORY_PROC 3
#define ONCRPC_LGE_ATCMD_MSG_PROC 4
#define ONCRPC_LGE_ATCMD_MSG_RSTSTR_PROC 5
#define ONCRPC_LGE_ATCMD_FACTORY_LARGE_PROC 6
#define ONCRPC_LGE_GET_FLEX_MCC_PROC 7 
#define ONCRPC_LGE_GET_FLEX_MNC_PROC 8 
#define ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC 9 
#define ONCRPC_LGE_GET_FLEX_COUNTRY_CODE_PROC 10

#define DSATAPIPROG		0x30000074
#define DSATAPIVERS		0

#define ONCRPC_DSATAPI_NULL_PROC 0
#define ONCRPC_DSAT_CHANGE_BAUD_PROC 1
#define ONCRPC_DSAT_GET_BAUD_PROC 2
#define ONCRPC_DSAT_GET_SREG_VAL_PROC 3
#define ONCRPC_DSAT_AT_CMD_TEST 4


struct rpc_misc_apps_bases_args {
	uint32_t at_cmd;
	uint32_t at_act;
	uint32_t at_param;
};

struct rpc_misc_apps_LARGE_bases_args {
	uint32_t at_cmd;
	uint32_t at_act;
	uint32_t sendNum;
	uint32_t endofBuffer;
	uint32_t buffersize;
	AT_SEND_BUFFER_t buffer[MAX_SEND_SIZE_BUFFER];
	
};

static struct atcmd_dev *atpdev;

//extern void update_battery_status_to_android(void);//temp
// Commands

// at_cmd value start
// !!! must same with dsatfactory.h in ARM9 (AMSS) side  !!!
//////////////////////////////////////////////////////////////////
#define ATCMD_AT	1
//Information Check
#define ATCMD_FRST	2
#define ATCMD_SWV	3
#define ATCMD_INFO	4
#define ATCMD_IMEI	5
#define ATCMD_IMPL	6
#define ATCMD_SLEN	7
#define ATCMD_SCHK	8
#define ATCMD_SULC	9
#define ATCMD_IDDE	10
#define ATCMD_HLRV	11
#define ATCMD_ISSIM	12
#define ATCMD_FUSG	13
#define ATCMD_DRMCERT	14
#define ATCMD_DRMTYPE	15
#define ATCMD_DRMERASE	16
#define ATCMD_INITDB	17
#define ATCMD_SVN	18
#define ATCMD_HWVER	19
// Calibration & Auto Test
#define ATCMD_CAMP	20
#define ATCMD_BNDI	21
#define ATCMD_CALM	22
#define ATCMD_NETORD	23
#define ATCMD_NETMODE	24
#define ATCMD_CALCK	25
#define ATCMD_CALDT	26
#define ATCMD_TESTCK	27
#define ATCMD_RADCK	28
#define ATCMD_RESTART	29
#define ATCMD_SFTUNE	30
// Call & Special Function Test
#define ATCMD_ACS	31
#define ATCMD_ECALL	32
#define ATCMD_FKPD	33
#define ATCMD_GKPD	34
#define ATCMD_MID	35
#define ATCMD_VLC	36
#define ATCMD_BATI	37
#define ATCMD_BATL	38
#define ATCMD_ANTB	39
#define ATCMD_SPM	40
#define ATCMD_FMT	41
#define ATCMD_FMR	42
#define ATCMD_MPT	43
#define ATCMD_MPC	44
#define ATCMD_AVR	45
#define ATCMD_EMT	46
#define ATCMD_INISIM	47
#define ATCMD_IRDA	48
#define ATCMD_PSENT	49
#define ATCMD_BATC	50
// Bluetooth Test	
#define ATCMD_BDCK	51
#define ATCMD_BTAD	52
#define ATCMD_BTTM	53 // hotaek 20070608 for BTTM
// DMB Test
#define ATCMD_DVBH	54
#define ATCMD_TDMB	55
#define ATCMD_CAS	56
// Misc
#define ATCMD_SYNC	57
#define ATCMD_BOFF	58//hotaek 2007/1/8 BOFF 
#define ATCMD_FC	59
#define ATCMD_FO	60
#define ATCMD_CONFIG	61
#define ATCMD_NDER	62
#define ATCMD_SPCK	63
#define ATCMD_SPUC	64
#define ATCMD_BCPL	65
#define ATCMD_SIMOFF	66
#define ATCMD_CGATT	67
#define ATCMD_RNO	68
#define ATCMD_LCD	69
#define ATCMD_CAM	70
#define ATCMD_MOT	71
// Wireless LAN
#define ATCMD_WLAN	72
#define ATCMD_WLANT	73
#define ATCMD_WLANR	74
// PMIC
#define ATCMD_PMRST	75
// Wireless LAN extention
#define ATCMD_WLANRF	76
#define ATCMD_WLANCAL	77
#define ATCMD_WLANWL	78

#define ATCMD_ATD	  79
#define ATCMD_MTC	  80

#define ATCMD_DRMINDEX		81
#define ATCMD_FLIGHT		82
#define ATCMD_LANG			83
// yorong drm command
#define ATCMD_DRMIMEI		84
#define ATCMD_POWERDOWN		85 

#define ATCMD_QEM		86

#define ATCMD_FRSTSTATUS			87
#define ATCMD_NOT_ATMCD_EARSENSE			88

//////////////////////////////////////////////////////////////////
// at_cmd value end



// at_act value  start
// !!! must same with dsatfactory.h in ARM9 (AMSS) side  !!!
//action/query/range/assign
//////////////////////////////////////////////////////////////////
#define ATCMD_ACTION	0
#define ATCMD_QUERY		1
#define ATCMD_RANGE		2
#define ATCMD_ASSIGN	3
//////////////////////////////////////////////////////////////////
// at_act value  end

#define OTHER_FB_TIMEOUT 3000
#define ORG_FB_TIMEOUT 21000

struct timer_list lg_fb_control;
static int fb_control_timer_init=0;
extern int msm_fb_refesh_enabled;
static void lg_fb_control_timer(unsigned long arg)
{
	printk(KERN_INFO "%s is expire\n",__func__);
	msm_fb_refesh_enabled=1;
}



// dsatHandleAT_ARM11 return Value
//////////////////////////////////////////////////////////////////
#define HANDLE_OK  0
#define HANLDE_FAIL 1
#define HANDLE_ERROR 2
#define HANDLE_OK_MIDDLE 4
//////////////////////////////////////////////////////////////////
/* constants relating to events sent into the input core */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <asm/gpio.h>
#include <mach/msm_i2ckbd.h>
#include <linux/delay.h>
//#include <asm/arch/lprintk.h>
#include <linux/at_kpd_eve.h> 
#include <mach/msm_rpcrouter.h> 

static char s_bufFlexINI[500];
static char s_bufItem[500];
static char s_bufValue2[400];
static char s_bufValue3[100];

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
static unsigned char at_fkpd_keycode[42]  = { //Encoder from AT cmd ASCII to Android key map code for GKPD
KEY_VOLUMEUP,	KEY_VOLUMEDOWN,	KEY_HOME,	KEY_SEARCH,	KEY_CAMERA,	242 /* FOCUS */,	KEY_6,	KEY_7,	KEY_8,	KEY_9,	/*#*/228,	227/***/, KEY_SEND, KEY_MENU,
85,			68,			72,			65,			67,			70,			54,			55,			56,			57,			35,			42,			83,				77,
41,			33,			64,			126,			36,			37,			60,			38,			62,			40,			63,			124,			115,				101
};
#else
static unsigned char at_fkpd_keycode[42]  = { //Encoder from AT cmd ASCII to Android key map code for GKPD
KEY_0,	KEY_1,	KEY_2,	KEY_3,	KEY_4,	KEY_5,	KEY_6,	KEY_7,	KEY_8,	KEY_9,	/*#*/228,	227/***/, KEY_SEND, KEY_END,
48,			49,			50,			51,			52,			53,			54,			55,			56,			57,			35,			42,			83,				69,
41,			33,			64,			126,			36,			37,			60,			38,			62,			40,			63,			124,			115,				101
};
#endif

enum kbd_inevents {
	QKBD_IN_KEYPRESS        = 1,
	QKBD_IN_KEYRELEASE      = 0,
	QKBD_IN_MXKYEVTS        = 256
};


// static int fkpd_press_delay, fkpd_release_delay, fkpd_work_index =0,
static int gkpd_state, gkpd_last_index, gkpd_value[21],fkpd_temp_index=0,  fkpd_type=0;
static int fkpd_buffer[41] = { 200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200, 
	                  200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200, 200};
	                  
int at_gkpd_cfg(unsigned int type, int value) //0,0 : READ STATE, 0,1 : READ LEFT VALUE, 1,0 : WRITE DISABLE VALUE, 1,1 : WRITE ENABLE VALUE
{
	int i,retval;
	if (type == 1) {
		gkpd_value[0] = 0;
		gkpd_last_index = 0;
		retval = 0;
		if (value == 1) {
			gkpd_state = 1;				
		}		
		else {
			gkpd_state = 0;	
		}
	}
	else {
		if (value == 0) {
			retval = gkpd_state;
		}
		else {
			retval = gkpd_value[0];			
			for (i = 0; i < gkpd_last_index; i++){
				gkpd_value[i] = gkpd_value[i+1];
			}
			gkpd_value[gkpd_last_index] = 0;
			if ( retval == 0 ) gkpd_last_index = 0;			
		}
	}
	return retval;
}
void write_gkpd_value(int value)
{
	int i;
	if (gkpd_state == 1) {
		if ( value != 0xff ) { 
			if (gkpd_last_index == 20) {
				gkpd_value[gkpd_last_index] = value;
				for ( i = 0; i < 20 ; i++) {
					gkpd_value[i] = gkpd_value[i+1];
				}			
				gkpd_value[gkpd_last_index] = 0;
			}
			else {
				gkpd_value[gkpd_last_index] = value;
				gkpd_value[gkpd_last_index+1] = 0;
				gkpd_last_index ++;
			}
		}		
	}	
}

int fkpd_xlkcode (int value)
{
	int i,index = 99;
	for ( i = 14; i < 42; i++){
			if ( value == at_fkpd_keycode[i] ){
				index = i;
			}
		}
	return index;	
}
void manual_fkpd_input (struct input_dev *dev, int value)
{		
		int index;		
		index = fkpd_xlkcode(value);
		printk("manual fkpd %d %c\n",at_fkpd_keycode[index-14], value);
		if ( index < 28 ) {
			input_report_key(dev, at_fkpd_keycode[index-14], QKBD_IN_KEYPRESS);
			input_sync(dev);
			mdelay (100);
		}
		else if ( index >27 && index < 42 ) {
			input_report_key(dev, at_fkpd_keycode[index-28], QKBD_IN_KEYRELEASE);
			input_sync(dev);
			mdelay (100);
		}
		else {
			printk("manual input error%d\n",value);
		}				
}

void auto_fkpd_input (struct input_dev *dev, int value, int time1, int time2)
{
		//struct input_dev *idev = pp2106m2_kbd_dev; 
		int index;
		index = fkpd_xlkcode(value);
		printk("auto fkpd %d %c\n",at_fkpd_keycode[index-14], value);
		if ( index < 28 ) {
			input_report_key(dev, at_fkpd_keycode[index-14], QKBD_IN_KEYPRESS);
			input_sync(dev);
			mdelay (time1*100);
			input_report_key(dev, at_fkpd_keycode[index-14], QKBD_IN_KEYRELEASE);
			input_sync(dev);
			mdelay (time2*100);
		}
		else {
			printk("auto input error%d\n",value);
		}				
}

extern struct input_dev *at_input_dev;


static void at_fkpd_fetchkeys (struct work_struct *work)
{		
	struct input_dev *idev = at_input_dev; 
	int i, press_delay=0, release_delay=0, type = 0;
	printk (" antispoon \n");
	mdelay(800);
	for (i=0;i<41;i++) printk("[%d]",fkpd_buffer[i]); //test antispoon
	//printk (" %dst antispoon \n",i);
	//for ( j =0; j < 2 ; j++) {
		for ( i = 0; i < 40 ; i++ ) { printk ("type[%d]buffer[%d]\n",type,fkpd_buffer[i]);
			if ( fkpd_buffer[i] != 200) {
				if ( type == 0 ) {
					press_delay = fkpd_buffer[i];
					fkpd_buffer[i] = 200;
					type =1;
				}
				else if ( type == 1 ) {
					release_delay = fkpd_buffer[i];
					fkpd_buffer[i] = 200;
					type = 2;
				}
				else if ( type == 2 ) {
					if ((press_delay == 0) && (release_delay == 0)) {
						fkpd_buffer[i] = 200;
						type = 3;	
					}
					else {
						fkpd_buffer[i] = 200;
						type = 4;	
					}
				}
				else if (type == 3) {
					manual_fkpd_input ( idev, fkpd_buffer[i] );	
					fkpd_buffer[i] = 200;
					type = 5;
				}
				else if (type ==4) {
					if ( fkpd_buffer[i] == 34 ){
						fkpd_buffer[i] = 200;
						type =0;	
					}
					else {
						auto_fkpd_input ( idev, fkpd_buffer[i], press_delay, release_delay );
						fkpd_buffer[i] = 200;
						type = 4;
					}
				}
				else if (type == 5) {
					fkpd_buffer[i] = 200;
					type = 0;	
				}
				else {
					//printk("out of gkpd\n");
					//fkpd_buffer[i] = 200;
					//type = 0;
				}
			}	
		}			
	for (i=0;i<41;i++) printk("[%d]",fkpd_buffer[i]); //test antispoon
	//}
}

DECLARE_WORK(fkpd_work, at_fkpd_fetchkeys); //antispoon test


int at_fkpd_cfg ( unsigned int type, int value)
{	
	printk (" type=%d value=%d fk_index=%d fk_type=%d\n",type,value,fkpd_temp_index,fkpd_type); //antispoon test
	fkpd_buffer[fkpd_temp_index] = value;
	fkpd_buffer[fkpd_temp_index+1] = 200;
	if (type == 0 ) {
		fkpd_type = 0;
		fkpd_temp_index = 0;
		fkpd_buffer[0]=200;
	}
	else {			
		if ( (value == 34) && (fkpd_type == 0) ) {
			//set_slide_open(0);  //antispoon
			fkpd_type = 1;	
		}
		else if ( (value == 34) && (fkpd_type == 1) ) {
			//fkpd_buffer[fkpd_temp_index] = 199;
			fkpd_type = 0;	
			schedule_work(&fkpd_work);  //antispoon0717_test
		}
		else {
			//fkpd_type = 1; 
			if ( value == 199 ){
				fkpd_type = 0;	
				fkpd_buffer[fkpd_temp_index] = 200;
				fkpd_temp_index = 0; //antispoon
				
			}
		}		
	
		if ( fkpd_temp_index < 39 ) {
			fkpd_temp_index++;
		}
		else {
			fkpd_temp_index = 0;	
		}
	}
	return 1; 
}	


// change little Endian to Big endian (there is not change function in Kernel)
char cpu_to_be8_AT(char value)
{
#define BITS_NUM_PER_BYTE  8
char c_value = 0;
#if 0

int loop = 0;
int TOT_SIZE = sizeof(char)*BITS_NUM_PER_BYTE;

for (loop = 0; loop < TOT_SIZE/2; loop++)
{
c_value |= (value & (1 << loop)) << (TOT_SIZE - loop -1);

}
// there is no ODD lengh 
for (loop = TOT_SIZE/2; loop < TOT_SIZE; loop++)
{
c_value |= (value & (1 << loop)) >> (TOT_SIZE/2 - loop +1 );

}
#else
c_value = value;
#endif
return c_value;
}

#define CAM_AVR_INITIAL     0
#define CAM_AVR_ON          1
#define CAM_AVR_SHOT        2
#define CAM_AVR_SAVE        3
#define CAM_AVR_CALL        4
#define CAM_AVR_ERASE       5
#define AVR_FLASH_ON        6
#define AVR_FLASH_OFF       7
#define CAM_STROBE_ON       8
#define CAM_STROBE_OFF      9
#define CAM_ZOOM_ON         11
#define CAM_ZOOM_OFF        12

void lge_atcmd_report_key(unsigned int, unsigned int);

#ifdef CONFIG_BACKLIGHT_EVE
void lge_atcmd_boff(void);
#endif

int external_memory_test(void)
{
	int return_value = 0;
	char *src = 0;
	char *dest=0;
	off_t fd_offset;
	int fd;
	
	if ( (fd = sys_open((const char __user *) "/sdcard/SDTest.txt", O_CREAT | O_RDWR, 0) ) < 0 )
	{
		printk(KERN_ERR "[ATCMD_EMT] Can not access SD card\n");
		goto file_fail;
	}

	src = kmalloc(10, GFP_KERNEL);
	sprintf(src,"TEST");
	if((sys_write(fd, (const char __user *) src, 5)) < 0)
	{
		printk(KERN_ERR "[ATCMD_EMT] Can not write SD card \n");
		goto file_fail;
	}
	fd_offset = sys_lseek(fd, 0, 0);
	
	dest = kmalloc(10, GFP_KERNEL);
	if((sys_read(fd, (char __user *) dest, 5)) < 0)
	{
		printk(KERN_ERR "[ATCMD_EMT]Can not read SD card \n");
		goto file_fail;
	}
	if ((memcmp(src, dest, 4)) == 0)
		return_value = 1;
	else 
		return_value = 0;

	file_fail:
	if ( src != 0 )
		kfree(src);
	if ( dest != 0 )
		kfree(dest);
		
	sys_close(fd);
	sys_unlink((const char __user *)"/sdcard/SDTest.txt");
	return return_value;
}

static int set_kernel_factory_reset_status(uint32_t status)
{
	int fd;
	char *factory_reset_status_buff = 0;
	int result = 0;
	if ( ( fd = sys_open((const char __user *) "/data/nv/FRStatus.txt", O_CREAT | O_RDWR, 0 )) < 0)
	{
		printk(KERN_ERR "[ATCMD_FRSTATUS] Can not access FRStatus.txt\n");
		goto file_set_frstatus_fail;
	}
	factory_reset_status_buff = kmalloc(2, GFP_KERNEL);
	sprintf(factory_reset_status_buff,"%d", status);
	if ( ( sys_write(fd, (const char __user *) factory_reset_status_buff, 1)) < 0 )
	{
		printk(KERN_ERR "[ATCMD_FRSTATUS] Can not write FRStatus.txt\n");
		goto file_set_frstatus_fail;
	}
	result = 1;
file_set_frstatus_fail:
	if ( factory_reset_status_buff != 0 )
		kfree(factory_reset_status_buff);
	
	sys_close(fd);
	return result;
}

static int get_kernel_factory_reset_status()
{
	int fd;
	char *factory_reset_status_buff = 0;
	int result = 0;
	if ( ( fd = sys_open((const char __user *) "/data/nv/FRStatus.txt", O_CREAT | O_RDWR, 0 )) < 0)
	{
		printk(KERN_ERR "[ATCMD_FRSTATUS] Can not access FRStatus.txt\n");
		goto file_get_frstatus_fail;
	}
	factory_reset_status_buff = kmalloc(2, GFP_KERNEL);
	if ( ( sys_read(fd, (const char __user *) factory_reset_status_buff, 1)) < 0 )
	{
		printk(KERN_ERR "[ATCMD_FRSTATUS] Can not read FRStatus.txt\n");
		goto file_get_frstatus_fail;
	}
	result = *factory_reset_status_buff - '0';
file_get_frstatus_fail:
	if ( factory_reset_status_buff != 0 )
		kfree(factory_reset_status_buff);
	
	sys_close(fd);
	return result;
}

extern unsigned char headset_inserted;


// at_param shall be ONLY  INTERGER, do not  (float / pointer / string...)
static int  dsatHandleAT_ARM11(uint32_t at_cmd, uint32_t at_act, uint32_t at_param,struct msm_rpc_server *server)
{
	int result = HANDLE_OK;
#ifdef USE_REPLY_RETSTRING
	int loop = 0;
#endif
	char ret_string[MAX_STRING_RET];
	uint32_t ret_value1 =0;
	uint32_t ret_value2 = 0;
	memset (ret_string, 0, sizeof(ret_string));


	switch (at_cmd)
	{
		// example code
		case ATCMD_FRST:
		{
			if(at_act != ATCMD_ACTION)
				result = HANLDE_FAIL;

			sprintf(ret_string, "edcb");
			ret_value1 = 10;
			ret_value2 = 20;
		}
		break;

		case ATCMD_SWV:
		{


		}
		break;

                case ATCMD_AVR: /* 45 */
                {
                        //printk("#### ATCMD_AVR ###### Act: 0x%x, Param: 0x%x\n", at_act, at_param);
			 printk("\n[kschoi] ***** ATCMD_AVR key = %d : rpc_server_misc.c",at_param);	
	         lge_atcmd_report_key(1, at_param);			 
                }
                break;
		case ATCMD_EMT:	//46
		{
			ret_value1 = external_memory_test();
		}
		break;
		
		case ATCMD_NOT_ATMCD_EARSENSE:
		{
		  printk(" ****************** ATCMD_NOT_ATMCD_EARSENSE ****************** \n");
		  printk(" ****************** headset_inserted [%d] ****************** \n",headset_inserted);
			ret_value1 = headset_inserted;
			break;
		}
		case ATCMD_FRSTSTATUS:
			if ( at_act == ATCMD_ACTION )
				ret_value1 = set_kernel_factory_reset_status(at_param);
			else
				ret_value1 = get_kernel_factory_reset_status();
			break;
			
       case ATCMD_MOT:	// 71
       {
       	
#if 1 // def CONFIG_ANDROID_VIBRATOR

       	if (at_act == ATCMD_ACTION){
       		ret_value1 = is_vib_state(); // Check vibrator state
          }
          else if (at_act == ATCMD_ASSIGN) {
				if (at_param== 1) 	{
//					vibrator_set(100);
					vibrator_enable();  // Enable vibrator
					ret_value1 = 1; 	// "MOTOR ON"
				}
				else	{
					vibrator_disable();  // Disabe vibrator
					printk("#### ATCMD_MOT 0###### Act: 0x%x, Param: 0x%x\n", at_act, at_param);
					ret_value1 = 0; 	// "MOTOR OFF"
				}
			}
			else	
#endif

			{
				result = HANLDE_FAIL;
			}
		}
		break;			
		case ATCMD_FKPD:	// 33       	
		{
          if (at_act == ATCMD_ASSIGN) {
				ret_value1 = 1; 
				at_fkpd_cfg(1/*ENABLE FKPD KEY INPUT*/, at_param/*AT INPUT KEY VALUE*/); 

			}
			else if (at_act == ATCMD_QUERY){		//JUST RETURN OK
				ret_value1 = 1;
			}
			else	{
				result = HANLDE_FAIL;
			}
		}
		break;			
			
		case ATCMD_GKPD:	// 34
		{
       	if (at_act == ATCMD_ACTION) {
 				
       		ret_value1 = at_gkpd_cfg(0/* 0 = READ GKPD*/,0/* READ GKPD STATE*/); //READ GKPD STATE
          }
          else if (at_act == ATCMD_ASSIGN) {
				if (at_param== 1) 	
					{
					at_gkpd_cfg(1/*WRITE GKPD*/,1/*WRITE TO SET GKPD BUFF*/); //SET GKPD STATE
					ret_value1 = 1; 	// "GKPD ON"
				}
				else	{
					at_gkpd_cfg(1/*WRITE GKPD*/,0/*DISABLE GKPD BUFF*/);  //DISABLE GKPD STATE
					ret_value1 = 0; 	// "GKPD OFF"
				}
			}
			else if (at_act == ATCMD_QUERY) {	
				ret_value1 = at_gkpd_cfg(0/*READ GKPD*/,1/*READ GKPD BUUFERED VALUE*/); //READ A GKPD BUUFER VALUE
			}
			else	{
				result = HANLDE_FAIL;
			}
		}
			break;

		case ATCMD_BOFF: // 58
		{			
			// To use AT%LCD, uncomment this.

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
			int fd;
			char buf = '0';
			mm_segment_t oldfs;

			oldfs = get_fs();
			set_fs(get_ds());

			fd = sys_open((const char __user *) "/sys/devices/virtual/backlight/rt9393/brightness", O_WRONLY | O_CREAT | O_TRUNC, 0777);
			if (fd < 0) {
				break;
			}

			sys_write(fd, &buf, sizeof(char));
			sys_close(fd);
			set_fs(oldfs);
#endif

#ifdef CONFIG_BACKLIGHT_EVE
			lge_atcmd_boff();
#endif
			ret_value1 = 1;
		}
			break;

		case ATCMD_FC:	// 59
			if(at_act != ATCMD_ACTION)
				result = HANLDE_FAIL;

#ifdef CONFIG_ANDROID_HALL_IC
			if (is_slide_open()== 0) 
			{
				set_slide_open(1);  // close slide
				ret_value1 = 1; 	// "FOLDER CLOSE OK"
			}
			else
			{
				ret_value1 = 0; 	// "FOLDER IS ALREADY CLOSED"
			}
#endif
			break;

		case ATCMD_FO:	// 60
			if(at_act != ATCMD_ACTION)
				result = HANLDE_FAIL;

#ifdef CONFIG_ANDROID_HALL_IC
			if (is_slide_open() == 0)
			{
				ret_value1 = 1; 	// "FOLDER IS ALREADY OPEN"
			}
			else
			{
				set_slide_open(0);  // open slide
				ret_value1 = 0; 	// "FOLDER OPEN OK"
			}
#endif
			break;
#if 1 // defined (CONFIG_MACH_EVE)-comment
		case ATCMD_VLC:	//36
			if(at_act != ATCMD_ACTION)
				result = HANLDE_FAIL;
			if (atpdev != NULL){
	 			update_atcmd_state(atpdev, "vlc", at_param); //state is up? down?
			}
			else 
			{
				printk("\n[%s] error vlc", __func__ );
			}
			break;
		case ATCMD_SPM:	//40
			if(at_act != ATCMD_ACTION)
				result = HANLDE_FAIL;
			if (atpdev != NULL){
	 			update_atcmd_state(atpdev, "spm", at_param); //state is up? down?
			}
			else 
			{
				printk("\n[%s] error spm", __func__ );
			}
			break;
			
		case ATCMD_ACS:
			if ( at_param != 0 )
				mdelay(1000);
			break;
		case ATCMD_MPT:	//43
			if(at_act != ATCMD_ACTION)
				result = HANLDE_FAIL;
			if (atpdev != NULL){
	 			update_atcmd_state(atpdev, "mpt", at_param); //state is up? down?
	 			mdelay (1000);
			}
			else 
			{
				printk("\n[%s] error mpt", __func__ );
			}
			break;
		case ATCMD_FMR: //42
		       printk("\n[rpc_server_misc] ATCMD_FMR");
			if(at_act != ATCMD_ACTION)
				result = HANLDE_FAIL;
			if (atpdev != NULL){
	 			update_atcmd_state(atpdev, "fmr", at_param); //state is up? down?
			}
			else 
			{
				printk("\n[%s] error fmr", __func__ );
			}
			break;
#endif
        case ATCMD_LCD: /* 69 */
			{	           
	            // To use AT%LCD, uncomment this.
	            lge_atcmd_report_key(ATCMD_LCD_REPORTKEY, at_param);				

				ret_value1 = 1;
        	}
            break;
        case ATCMD_CAM: /* 70 */
	        {
	                //printk("#### ATCMD_CAM ###### Act: 0x%x, Param: 0x%x\n", at_act, at_param);
					printk("\n[kschoi] ***** ATCMD_CAM key = %d: rpc_server_misc.c",at_param);
	                lge_atcmd_report_key(0, at_param);					
	        }
        	break;

		default :
			result = HANDLE_ERROR;
			break;


	}

	// give to RPC server result
	/////////////////////////////////////////////////////////////////
#ifdef USE_REPLY_RETSTRING
	for(loop = 0; loop < MAX_STRING_RET ; loop++)
	{
		server->retvalue.ret_string[loop]= (AT_STR_t)(ret_string[loop]);
	}
	server->retvalue.ret_string[MAX_STRING_RET-1] = 0;
	server->retvalue.ret_value1 = ret_value1;
	server->retvalue.ret_value2 = ret_value2;
#endif
/////////////////////////////////////////////////////////////////
	return result;
}

int lg_get_flex_from_xml(char *strIndex, char* flexValue)
{
	int                       fd;
	int                       iFlexSize;
	signed long int      res;
	int                       iTotalCnt, iItemCnt,iValueCnt,j,iItemCntAll;

		
//	char*                    s_bufFlexINI=0; //Flex INI ?????? ??ü string
//	char                    s_bufItem[500];     //one line
	// char                    s_bufValue1[10];   //country
//	char                    s_bufValue2[400]; //value
//	char                    s_bufValue3[100]; //ID
	int response = 0;


	iFlexSize = 500;	
	fd = sys_open((const char __user *) "/system/etc/flex/flex.xml", O_RDONLY,0);

	if (fd == -1)	
		{
		printk("read data fail");
		return 0;
		}

	//iFlexSize = (unsigned)sys_lseek(fd, (off_t)0, 0);
//	s_bufFlexINI = kmalloc(500, GFP_KERNEL);
	//memset(s_bufFlexINI,0x00,sizeof(char)*iFlexSize);	
	res = sys_read(fd, (char __user*)s_bufFlexINI, sizeof(char)*iFlexSize);
	
	sys_close(fd);

	printk("read data flex.xml fd: %d iFlexSize : %d res;%d\n", fd, iFlexSize, res);

	iFlexSize=res;
	
	iItemCnt = 0;
	iItemCntAll = 0;
	
	for(iTotalCnt=0; iTotalCnt<iFlexSize;iTotalCnt++)  //Flex ini ?????? ?? character ??n?1?
	{
		//printk("%x ",s_bufFlexINI[iTotalCnt]);
		if ((s_bufFlexINI[iItemCntAll]) != '\n')  //???ڿ?; ???? ?о??????ۿ? ?ֱ?
		{
			s_bufItem[iItemCnt]=s_bufFlexINI[iItemCntAll];
			iItemCnt ++;
			
		} 
		else  //?о??? ???ڿ? ?м?
		{	
			//printk("\n",s_bufFlexINI[iTotalCnt]);
			s_bufItem[iItemCnt]='\n';
				
			j = 0;
			iValueCnt = 0;
			memset(s_bufValue3,0x00,sizeof(char)*100);
			while((s_bufItem[j] != '=') && (s_bufItem[j] != '\n') /*&& (s_bufItem[j] != ';')*/)  
			{
				//printk("\ns_bufValue3 ");
				if(s_bufItem[j] != ' ' && s_bufItem[j] != '\t') 
				{
					s_bufValue3[iValueCnt++] = s_bufItem[j];
					//printk("%x ",s_bufValue3[iValueCnt]);
				}
				j++;
			}

			if(!strncmp(s_bufValue3,strIndex,strlen(strIndex)))
			{ 
				printk("find %s ",strIndex);
				iValueCnt = 0;
				j++;
				while(s_bufItem[j] != '\n' )  
				{
					//printk("\ns_bufItem ");
					if(s_bufItem[j] != '"') 
					{
						s_bufValue2[iValueCnt++] = s_bufItem[j];
						printk("%x ",s_bufValue2[iValueCnt]);
					}
					j++;
				}
				memcpy(flexValue,s_bufValue2, iValueCnt);
				if(flexValue[iValueCnt-1] == '\r')
				{
					flexValue[iValueCnt-1] = 0x00;
					if(iValueCnt == 1)
					{
						printk("\niValueCnt == 1");
						response =  0;
						goto lg_get_flex_fail;
					}
				}
				else
				{
					flexValue[iValueCnt] = 0x00;
					if(iValueCnt == 0)
					{
						printk("\niValueCnt == 0");
						response = 0;
						goto lg_get_flex_fail;
					}
				}
				
				response = 1;
				goto lg_get_flex_fail;
			}
			iItemCnt = 0;
		}
		iItemCntAll++;
	}


lg_get_flex_fail:
//	if ( s_bufFlexINI != 0 )
//		kfree(s_bufFlexINI);


	return response;

}

static int eta_execute(char *string)
{
	int ret;
	char cmdstr[100];
	int fd;
	char *envp[] = {
		"HOME=/",
		"TERM=linux",
		NULL,
	};
	if ( (fd = sys_open((const char __user *) "/system/bin/eta", O_RDONLY ,0) ) < 0 )
	{
			printk("\n can not open /system/bin/eta - execute /system/bin/eta\n");
			sprintf(cmdstr, "/system/bin/eta %s", string);
	}
	else
	{
		printk("\n execute /system/bin/eta\n");
		sprintf(cmdstr, "/system/bin/eta %s", string);
		sys_close(fd);
	}
	char *argv[] = {
		"sh",
		"-c",
		cmdstr,
		NULL,
	};

printk(KERN_INFO "execute eta : data - %s", cmdstr);
	if ((ret =
	     call_usermodehelper("/system/bin/sh", argv, envp, UMH_WAIT_PROC)) != 0) {
		printk(KERN_ERR "Eta failed to run \": %i\n",
		       ret);
	}
	else
		printk(KERN_INFO "execute ok");
	return ret;
}
static int  dsatHandleAT_ARM11_LARGE(struct rpc_misc_apps_LARGE_bases_args *args,struct msm_rpc_server *server)
{

int result = HANDLE_OK;
int loop = 0;

char ret_string[MAX_STRING_RET];
uint32_t ret_value1 =0;
uint32_t ret_value2 = 0;
static AT_SEND_BUFFER_t totalBuffer[LIMIT_MAX_SEND_SIZE_BUFFER];
static uint32_t totalBufferSize = 0;
uint32_t at_cmd,at_act;
memset (ret_string, 0, sizeof(ret_string));

// init for LARGE Buffer
if(args->sendNum == 0)
{
// init when first send
memset(totalBuffer, 0, sizeof(totalBuffer));
totalBufferSize = 0;

}
		args->at_cmd = be32_to_cpu(args->at_cmd);
		args->at_act = be32_to_cpu(args->at_act);
		args->sendNum = be32_to_cpu(args->sendNum);
		args->endofBuffer = be32_to_cpu(args->endofBuffer);
		args->buffersize = be32_to_cpu(args->buffersize);
		
		printk(KERN_INFO "handle_misc_rpc_call at_cmd = %d, at_act=%d, sendNum=%d:\n",
		      args->at_cmd, args->at_act,args->sendNum);
		printk(KERN_INFO "handle_misc_rpc_call endofBuffer = %d, buffersize=%d:\n",
		      args->endofBuffer, args->buffersize);
		printk(KERN_INFO "input buff[0] = %d,buff[1]=%d,buff[2]=%d:\n",args->buffer[0],args->buffer[1],args->buffer[2]);
		// printk("len = %d\n", len);
		if(args->sendNum < MAX_SEND_LOOP_NUM)
			{
			for(loop = 0; loop < args->buffersize; loop++)
				{
				// totalBuffer[MAX_SEND_SIZE_BUFFER*args->sendNum + loop] =  be32_to_cpu(args->buffer[loop]);
				totalBuffer[MAX_SEND_SIZE_BUFFER*args->sendNum + loop] =  (args->buffer[loop]);
				}
			
			// memcpy(totalBuffer + MAX_SEND_SIZE_BUFFER*args->sendNum, args->buffer, args->buffersize);
			totalBufferSize += args->buffersize;
			
			}
		printk(KERN_INFO "handle_misc_rpc_call buff[0] = %d,buff[1]=%d,buff[2]=%d:\n",
		      totalBuffer[0 + args->sendNum*MAX_SEND_SIZE_BUFFER], totalBuffer[1 + args->sendNum*MAX_SEND_SIZE_BUFFER], totalBuffer[2+args->sendNum*MAX_SEND_SIZE_BUFFER]);
		if(!args->endofBuffer )
			return HANDLE_OK_MIDDLE;

at_cmd = 	args->at_cmd;
at_act = args->at_act;

///////////////////////////////////////////////////
/* please use
static uint8_t totalBuffer[LIMIT_MAX_SEND_SIZE_BUFFER];
static uint32_t totalBufferSize = 0;
uint32_t at_cmd,at_act;
*/
///////////////////////////////////////////////////
switch (at_cmd)
{
// example code
case ATCMD_FRST:
{
	if(at_act != ATCMD_ACTION)
		result = HANLDE_FAIL;

	sprintf(ret_string, "edcb");
	ret_value1 = 10;
	ret_value2 = 20;



}
break;


case ATCMD_SWV:
{


}
break;
case ATCMD_MTC:
{
	int exec_result =0;

	printk(KERN_INFO "\nATCMD_MTC\n ");

	if(at_act != ATCMD_ACTION)
		result = HANLDE_FAIL;

	printk(KERN_INFO "totalBuffer : %s size: %d ", totalBuffer, totalBufferSize);
	exec_result = eta_execute(totalBuffer);
	printk(KERN_INFO "AT+MTC exec_result %d ",exec_result);
	sprintf(ret_string, "edcb");
	ret_value1 = 10;
	ret_value2 = 20;

}
break;

default :
	result = HANDLE_ERROR;
	break;


}

// give to RPC server result
/////////////////////////////////////////////////////////////////
#ifdef USE_REPLY_RETSTRING
for(loop = 0; loop < MAX_STRING_RET ; loop++)
{
server->retvalue.ret_string[loop]= (AT_STR_t)(ret_string[loop]);

}
server->retvalue.ret_string[MAX_STRING_RET-1] = 0;
server->retvalue.ret_value1 = ret_value1;
server->retvalue.ret_value2 = ret_value2;
if(args->endofBuffer )
{
// init when first send
memset(totalBuffer, 0, sizeof(totalBuffer));
totalBufferSize = 0;

}
#endif
/////////////////////////////////////////////////////////////////
return result;
}


static int handle_misc_rpc_call(struct msm_rpc_server *server,
			   struct rpc_request_hdr *req, unsigned len)
{
int result = RPC_ACCEPTSTAT_SUCCESS;
	atpdev = atcmd_get_dev();
	
	switch (req->procedure) {
	case ONCRPC_MISC_APPS_APIS_NULL_PROC:
		return 0;

	case ONCRPC_LCD_MESSAGE_PROC: 
			{
		printk(KERN_INFO"ONCRPC_LCD_MESSAGE_PROC\n");
		}
		return 0;

	case ONCRPC_LCD_DEBUG_MESSAGE_PROC:
			{
		printk(KERN_INFO"ONCRPC_LCD_MESSAGE_PROC\n");
		}

		return 0;
	case ONCRPC_LGE_ATCMD_FACTORY_PROC:
		{
		printk("ONCRPC_LGE_ATCMD_FACTORY_PROC\n");
		{
		struct rpc_misc_apps_bases_args *args;
		args = (struct rpc_misc_apps_bases_args *)(req + 1);
		args->at_cmd = be32_to_cpu(args->at_cmd);
		args->at_act = be32_to_cpu(args->at_act);
		args->at_param = be32_to_cpu(args->at_param);
		printk(KERN_INFO "handle_misc_rpc_call at_cmd = %d, at_act=%d, at_param=%d:\n",
		      args->at_cmd, args->at_act,args->at_param);
		// printk("len = %d\n", len);
		#ifdef USE_REPLY_RETSTRING
		 memset(server->retvalue.ret_string, 0, sizeof(server->retvalue.ret_string));
		//for(loop = 0; loop < MAX_STRING_RET; loop++)
		//server->retvalue.ret_string[loop] =0;
		//server->retvalue.ret_value1 = 0;
		//server->retvalue.ret_value2 = 0;
		#endif
		// LGE_CHAGE_E   princlee
		//printk(KERN_INFO"rpc_servers_thread rpc_vers=%d, prog=%d, vers=%d\n", req->rpc_vers,req->prog,req->vers);

		if(dsatHandleAT_ARM11(args->at_cmd, args->at_act, args->at_param,server) == HANDLE_OK)
			{
			
			result = RPC_RETURN_RESULT_OK;
			}
		else
			result= RPC_RETURN_RESULT_ERROR;
		
		return result;
		}
		}
	return 0;
	case ONCRPC_LGE_ATCMD_FACTORY_LARGE_PROC:
		{
		printk("ONCRPC_LGE_ATCMD_FACTORY_LARGE_PROC\n");
		{
		int loop = 0;
		int dsat_result = HANDLE_OK;
		struct rpc_misc_apps_LARGE_bases_args *args;
		args = (struct rpc_misc_apps_LARGE_bases_args *)(req + 1);
		#ifdef USE_REPLY_RETSTRING
		 memset(server->retvalue.ret_string, 0, sizeof(server->retvalue.ret_string));
		//for(loop = 0; loop < MAX_STRING_RET; loop++)
		//server->retvalue.ret_string[loop] =0;
		//server->retvalue.ret_value1 = 0;
		//server->retvalue.ret_value2 = 0;
		#endif
		// LGE_CHAGE_E   princlee
		//printk(KERN_INFO"rpc_servers_thread rpc_vers=%d, prog=%d, vers=%d\n", req->rpc_vers,req->prog,req->vers);

		dsat_result = dsatHandleAT_ARM11_LARGE(args,server);
		if(dsat_result == HANDLE_OK)
			{
			
			result = RPC_RETURN_RESULT_OK;
			}
		else if(dsat_result == HANDLE_OK_MIDDLE)
			{
			result = RPC_RETURN_RESULT_MIDDLE_OK;
			}
		else
			result= RPC_RETURN_RESULT_ERROR;
		
		return result;
		}
		}
	return 0;
	case ONCRPC_LGE_ATCMD_MSG_PROC:
			{
		printk(KERN_INFO"ONCRPC_LGE_ATCMD_MSG_PROC\n");
		}
	return 0;

	case ONCRPC_LGE_ATCMD_MSG_RSTSTR_PROC:
			{
		printk(KERN_INFO"ONCRPC_LGE_ATCMD_MSG_RSTSTR_PROC\n");
		}
	return 0;
	case ONCRPC_LGE_GET_FLEX_MCC_PROC	:
	{		
		printk(KERN_INFO"ONCRPC_LGE_GET_FLEX_MCC_PROC\n");
		memset(server->retvalue.ret_string, 0, sizeof(server->retvalue.ret_string));		
		if(lg_get_flex_from_xml("FLEX_MCC_CODE", server->retvalue.ret_string))
			result = RPC_RETURN_RESULT_OK;
		else
			result= RPC_RETURN_RESULT_ERROR;
		server->retvalue.ret_value1 = strlen(server->retvalue.ret_string);
		server->retvalue.ret_value2 = 0;
		printk(KERN_INFO "ONCRPC_LGE_GET_FLEX_MCC_PROC return string : %d , %s\n",
		      server->retvalue.ret_value1,server->retvalue.ret_string);		
		return result;
	}
	case ONCRPC_LGE_GET_FLEX_MNC_PROC	:
	{		
		printk(KERN_INFO"ONCRPC_LGE_GET_FLEX_MNC_PROC\n");
		memset(server->retvalue.ret_string, 0, sizeof(server->retvalue.ret_string));		
		if(lg_get_flex_from_xml("FLEX_MNC_CODE", server->retvalue.ret_string))
			result = RPC_RETURN_RESULT_OK;
		else
			result= RPC_RETURN_RESULT_ERROR;
		server->retvalue.ret_value1 = strlen(server->retvalue.ret_string);
		server->retvalue.ret_value2 = 0;
		printk(KERN_INFO "ONCRPC_LGE_GET_FLEX_MNC_PROC return string : %d , %s\n",
		      server->retvalue.ret_value1,server->retvalue.ret_string);	

		return result;
	}
	case ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC	:
	{		
		printk(KERN_INFO"ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC\n");
		memset(server->retvalue.ret_string, 0, sizeof(server->retvalue.ret_string));		
		if(lg_get_flex_from_xml("FLEX_OPERATOR_CODE", server->retvalue.ret_string))
			result = RPC_RETURN_RESULT_OK;
		else
			result= RPC_RETURN_RESULT_ERROR;
		server->retvalue.ret_value1 = strlen(server->retvalue.ret_string);
		server->retvalue.ret_value2 = 0;
		printk(KERN_INFO "ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC return string : %d , %s\n",
		      server->retvalue.ret_value1,server->retvalue.ret_string);	
#if 0					
		{
			extern int msm_fb_refesh_enabled;
			if (!msm_fb_refesh_enabled)
				msm_fb_refesh_enabled = 1;
		}
#endif

		
		if(!msm_fb_refesh_enabled && !fb_control_timer_init) {
			printk(KERN_INFO "%s: jori set timer\n",__func__);
			setup_timer(&lg_fb_control,lg_fb_control_timer,0);

			if(strncmp(server->retvalue.ret_string,"ORG",3)==0){
				mod_timer(&lg_fb_control,jiffies +(ORG_FB_TIMEOUT*HZ/1000));
			} else {
				mod_timer(&lg_fb_control,jiffies +(OTHER_FB_TIMEOUT*HZ/1000));
			}
			fb_control_timer_init = 1;
		}
		return result;
	}
	case ONCRPC_LGE_GET_FLEX_COUNTRY_CODE_PROC	:
	{		
		printk(KERN_INFO"ONCRPC_LGE_GET_FLEX_COUNTRY_PROC\n");
		memset(server->retvalue.ret_string, 0, sizeof(server->retvalue.ret_string));		
		if(lg_get_flex_from_xml("FLEX_COUNTRY_CODE", server->retvalue.ret_string))
			result = RPC_RETURN_RESULT_OK;
		else
			result= RPC_RETURN_RESULT_ERROR;
		server->retvalue.ret_value1 = strlen(server->retvalue.ret_string);
		server->retvalue.ret_value2 = 0;
		printk(KERN_INFO "ONCRPC_LGE_GET_FLEX_COUNTRY_PROC return string : %d , %s\n",
		      server->retvalue.ret_value1,server->retvalue.ret_string);		
		return result;
	}

//LGE_UPDATE_E
	default:
		return -ENODEV;
	}
}



static struct msm_rpc_server rpc_server = {

	.prog = MISC_APPS_APISPROG,
	.vers = MISC_APPS_APISVERS,
	.rpc_call = handle_misc_rpc_call,



};

static int __init rpc_misc_server_init(void)
{
	return msm_rpc_create_server(&rpc_server);
}


module_init(rpc_misc_server_init);

