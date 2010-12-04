/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/usb/mass_storage_function.h>
#include <linux/power_supply.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif

#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_rpcrouter.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/msm_serial_hs.h>
#include <mach/memory.h>
#include <mach/msm_battery.h>

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/i2c.h>
#include <linux/android_pmem.h>
#include <mach/camera.h>

#include <linux/eve_at.h>

#ifdef CONFIG_BATTERY_MSM
#include <linux/power_supply.h>
#include <mach/msm_battery.h>
#endif
#include "lge_charging_table.h"

#include <mach/board_swift.h>

#include "../devices.h"
#include "../socinfo.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#endif
#include "../pm.h"
#include <linux/msm_kgsl.h>

uint32_t g_fb_addr=0;
// MOD : [KERNEL] 5340 KERNEL PATCH
#ifdef CONFIG_ARCH_MSM7X27
#define MSM_PMEM_MDP_SIZE	0x1700000
#define MSM_PMEM_ADSP_SIZE	0xAE4000
#define MSM_PMEM_AUDIO_SIZE	0x5B000
#define MSM_FB_SIZE		0x177000
#define MSM_GPU_PHYS_SIZE	SZ_2M
#define PMEM_KERNEL_EBI1_SIZE	0x1C000
#endif

#if 0
#define MSM_PMEM_MDP_SIZE	0x1591000
#define MSM_PMEM_ADSP_SIZE	0x99A000//5C2000-->99A000
#if 0
#define MSM_PMEM_GPU1_SIZE	0x1000000
#endif
#define MSM_FB_SIZE		0x200000
#define MSM_GPU_PHYS_SIZE	SZ_2M

#define MSM_PMEM_AUDIO_SIZE    0x121000 

#define PMEM_KERNEL_EBI1_SIZE	0x80000
#endif
/* for MMC detect */
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION

#define GPIO_MMC_CD_N 49
#if defined(CONFIG_MACH_MSM7X27_SWIFT_REV_1)
#define GPIO_MMC_CD_COVER 19
#endif

#endif

/* initialize timed_output vibrator */
void __init msm_init_timed_vibrator(void);

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
#if defined(CONFIG_MACH_MSM7X27_SWIFT) && defined(CONFIG_LGE_USB_DRIVER)
	.nluns			= 0x01,
#else /* Below is original */
	.nluns          = 0x02,
#endif  /* CONFIG_MACH_MSM7X27_SWIFT */

	.buf_size       = 16384,

#if defined(CONFIG_MACH_MSM7X27_SWIFT) && defined(CONFIG_LGE_USB_DRIVER)
	.vendor         = "LGE",
	.product        = "Android Platform",
	.release        = 0xffff,
#else /* Below is original */
	.vendor         = "GOOGLE",
	.product        = "Mass storage",
	.release        = 0xffff,
#endif  /* CONFIG_MACH_MSM7X27_SWIFT */
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif
#ifdef CONFIG_USB_ANDROID
/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9015,
		/* MSC + ADB */
		.functions	    = 0x12 /* 10010 */
	},
	{
		.product_id         = 0xF000,
		/* MSC */
		.functions	    = 0x02, /* 0010 */
	},
	{
		.product_id         = 0xF005,
		/* MODEM ONLY */
		.functions	    = 0x03,
	},

	{
		.product_id         = 0x8080,
		/* DIAG + MODEM */
		.functions	    = 0x34,
	},
	{
		.product_id         = 0x8082,
		/* DIAG + ADB + MODEM */
		.functions	    = 0x0314,
	},
	{
		.product_id         = 0x8085,
		/* DIAG + ADB + MODEM + NMEA + MSC*/
		.functions	    = 0x25314,
	},
	{
		.product_id         = 0x9016,
		/* DIAG + GENERIC MODEM + GENERIC NMEA*/
		.functions	    = 0x764,
	},
	{
		.product_id         = 0x9017,
		/* DIAG + GENERIC MODEM + GENERIC NMEA + MSC*/
		.functions	    = 0x2764,
	},
	{
		.product_id         = 0x9018,
		/* DIAG + ADB + GENERIC MODEM + GENERIC NMEA + MSC*/
		.functions	    = 0x27614,
	},
	{
		.product_id         = 0xF009,
		/* CDC-ECM*/
		.functions	    = 0x08,
	}
};
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x05C6,
	.product_id	= 0x9018,
	.functions	= 0x27614,
	.version	= 0x0100,
	.compositions   = usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.product_name	= "Qualcomm HSUSB Device",
	.manufacturer_name = "Qualcomm Incorporated",
	.nluns = 1,
};
static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
#endif

#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
#if defined(CONFIG_MACH_MSM7X27_SWIFT) && defined(CONFIG_LGE_USB_DRIVER)
	{"modem", 0},
	{"diag", 1},
	{"nmea", 2},
	{"mass_storage", 3},
	{"adb", 4},
	{"ethernet", 5},
#else /* Below is original */
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
#endif  /* CONFIG_MACH_MSM7X27_SWIFT */
};

/* 2010-02-15, For NEW LGE host(PC) USB driver:
 *
 * Defined NEW USB connection modes for Android(by LGE)
 *
 * Normal mode(Full) : MODEM + DIAG + NMEA + UMS + ADB  (pid : 0x618E)
 * Normal mode(Light): MODEM + DIAG + NMEA + UMS        (pid : 0x618E)
 * Manufacturing mode: MODEM + DIAG                     (pid : 0x6000)
 * Mass Storage Only : UMS                              (pid : 0x61B4)
 *
 * As Full and Light modes have shared pid, we must use temporary swtiching.
 * For this, Light mode has temporary pid 0x6003. This pid is not used out
 * of phone and is in only HSUSB driver.
 */
/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
#if defined(CONFIG_MACH_MSM7X27_SWIFT) && defined(CONFIG_LGE_USB_DRIVER)
    { 
        .product_id     = 0x61B4, 
        .functions      = 0x8,  /* 001000 UMS only */  }, 
	{
		.product_id     = 0x6000,
		.functions      = 0x3,  /* 000011 MDM+DIAG */  },
	{
		.product_id     = 0x6003,
		.functions      = 0x0F, /* 001111 MDM+DIAG+GPS+UMS */   },
	{
		.product_id     = 0x618E,
		.functions      = 0x1F, /* 011111 MDM+DIAG+GPS+UMS+ADB */   },
#else /* Below is original */
	{	
		.product_id     = 0x9012,
		.functions	    = 0x5, /* 0101 */  	},
	{	
		.product_id     = 0x9013,
		.functions	    = 0x15, /* 10101 */ },
	{
		.product_id     = 0x9014,
		.functions	    = 0x30, /* 110000 */},
	{
		.product_id     = 0x9016,
		.functions	    = 0xD, /* 01101 */  },
	{
		.product_id     = 0x9017,
		.functions	    = 0x1D, /* 11101 */ },
	{
		.product_id     = 0xF000,
		.functions	    = 0x10, /* 10000 */ },
	{
		.product_id     = 0xF009,
		.functions	    = 0x20, /* 100000 */ },
	{
		.product_id     = 0x9018,
		.functions	    = 0x1F, /* 011111 */ },
#endif  /* CONFIG_MACH_MSM7X27_SWIFT */
};
#endif

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
#ifdef CONFIG_USB_FUNCTION
	.version		= 0x0100,
	.phy_info		= (USB_PHY_INTEGRATED | USB_PHY_MODEL_65NM),
#if defined(CONFIG_MACH_MSM7X27_SWIFT) && defined(CONFIG_LGE_USB_DRIVER)
	.vendor_id          = 0x1004,
	.product_name       = "LG Mobile USB Modem",
	.serial_number      = "LGE_ANDROID_GT540",
	.manufacturer_name  = "LG Electronics Inc.",
#else /* Below is original */
	.vendor_id          = 0x5c6,
	.product_name       = "Qualcomm HSUSB Device",
	.serial_number      = "1234567890ABCDEF",
	.manufacturer_name  = "Qualcomm Incorporated",
#endif  /* CONFIG_MACH_MSM7X27_SWIFT */
	.compositions	= usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.function_map   = usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.config_gpio    = NULL,
#endif
};

static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static int hsusb_chg_init(int connect)
{
	if (connect)
		return msm_chg_rpc_connect();
	else
		return msm_chg_rpc_close();
}

void hsusb_chg_vbus_draw(unsigned mA)
{
	if (mA)
		msm_chg_usb_i_is_available(mA);
	else
		msm_chg_usb_i_is_not_available();
}

void hsusb_chg_connected(enum chg_type chgtype)
{
	switch (chgtype) {
	case CHG_TYPE_HOSTPC:
		pr_debug("Charger Type: HOST PC\n");
		msm_chg_usb_charger_connected(0);
		msm_chg_usb_i_is_available(100);
		break;
	case CHG_TYPE_WALL_CHARGER:
		pr_debug("Charger Type: WALL CHARGER\n");
		msm_chg_usb_charger_connected(2);
		msm_chg_usb_i_is_available(1500);
		break;
	case CHG_TYPE_INVALID:
		pr_debug("Charger Type: DISCONNECTED\n");
		msm_chg_usb_i_is_not_available();
		msm_chg_usb_charger_disconnected();
		break;
	}
}

static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,
	.phy_reset	= msm_hsusb_phy_reset,
};

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	/* charging apis */
	.chg_init = hsusb_chg_init,
	.chg_connected = hsusb_chg_connected,
	.chg_vbus_draw = hsusb_chg_vbus_draw,
};

#define SND(desc, num) { .name = #desc, .id = num }
#if 1
static struct snd_endpoint snd_endpoints_list[] = {
#if 0
	SND(HANDSET, 0),
	SND(MONO_HEADSET, 2),
	SND(HEADSET, 3),
	SND(SPEAKER, 6),
	SND(TTY_HEADSET, 8),
	SND(TTY_VCO, 9),
	SND(TTY_HCO, 10),
	SND(BT, 12),
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(CURRENT, 27),
#else
/*
	SND(HANDSET, 5),
	SND(HEADSET_LOOPBACK, 1),
	SND(HEADSET, 2),
	SND(HEADSET_STEREO, 3),
	SND(SPEAKER, 0),
	SND(SPEAKER_IN_CALL, 6),
	SND(SPEAKER_RING, 7),
	SND(HEADSET_AND_SPEAKER, 7),
	SND(VOICE_RECORDER, 8),
	SND(FM_HEADSET, 9),
	SND(FM_SPEAKER, 10),
	SND(BT, 12),
	SND(TTY_HEADSET, 14),
	SND(TTY_VCO, 15),
	SND(TTY_HCO, 16),
	SND(CURRENT, 25),
*/
	SND(HANDSET, 0),
	SND(HEADSET_STEREO_AUDIO, 2),//31
	SND(TTY_HEADSET, 2),				// alohag
	SND(HEADSET_STEREO, 2),
	SND(HEADSET, 3),					// alohag
	SND(HEADSET_LOOPBACK, 3),		// alohag
	SND(SPEAKER_PHONE, 6),
	SND(SPEAKER_IN_CALL, 6),			// alohag
	SND(SPEAKER_AUDIO, 5),
	SND(SPEAKER, 5),					// alohag
	SND(SPEAKER_RING, 5),			// alohag

	SND(TTY_VCO, 5),					// alohag
	SND(TTY_HCO, 5),					// alohag

//	SND(HEADSET_SPEAKER, 7),
	SND(HEADSET_AND_SPEAKER, 7),
	SND(VOICE_RECORDER, 8),
	SND(FM_RADIO_HEADSET_MEDIA, 9),
	SND(FM_HEADSET, 9),				// alohag
	SND(FM_RADIO_SPEAKER_MEDIA, 10),
	SND(FM_SPEAKER, 10),				// alohag
	SND(BT, 12),
	SND(BT_A2DP, 11),//20
	SND(CURRENT, 35),	

#endif
};
#else
static struct snd_endpoint snd_endpoints_list[] = {


/*
#define		LGE_SND_DEVICE_HANDSET					0
#define		LGE_SND_DEVICE_STEREO_HEADSET				3    // FOR VOICE
#define		LGE_SND_DEVICE_STEREO_HEADSET_AUDIO		31  // FOR MEDIA
#define		LGE_SND_DEVICE_SPEAKER_AUDIO				5    // FOR VOICE
#define		LGE_SND_DEVICE_SPEAKER_PHONE				6    // FOR MEDIA
#define		LGE_SND_DEVICE_HEADSET_SPEAKER			7    // FOR both headset and speaker
#define		LGE_SND_DEVICE_VOICE_RECORDER				8    // FOR VOICE RECORDER
#define		LGE_SND_DEVICE_FM_RADIO_HEADSET_MEDIA	9    // FOR FM RADIO HEADSET MEDIA
#define		LGE_SND_DEVICE_FM_RADIO_SPEAKER_MEDIA		10   // FOR FM RADIO HEADSET MEDIA MULTI
#define		LGE_SND_DEVICE_BT_HEADSET					12   // FOR BT (SCO)
#define		LGE_SND_DEVICE_A2DP_HEADSET				20   // FOR BT (A2DP)
*/
#if 0
	SND(HANDSET, 0),
	SND(MONO_HEADSET, 2),
	SND(HEADSET, 3),
	SND(SPEAKER, 6),
	SND(TTY_HEADSET, 8),
	SND(TTY_VCO, 9),
	SND(TTY_HCO, 10),
	SND(BT, 12),
	SND(IN_S_SADC_OUT_HANDSET, 16),
	SND(IN_S_SADC_OUT_SPEAKER_PHONE, 25),
	SND(CURRENT, 27),
#else
	SND(HANDSET, 0),
	SND(HEADSET_STEREO_AUDIO, 2),//31
	SND(HEADSET_STEREO, 3),
	SND(SPEAKER_PHONE, 6),
	SND(SPEAKER_AUDIO, 5),
	SND(HEADSET_SPEAKER, 7),
	SND(VOICE_RECORDER, 8),
	SND(FM_RADIO_HEADSET_MEDIA, 9),
	SND(FM_RADIO_SPEAKER_MEDIA, 10),
	SND(BT, 12),
	SND(BT_A2DP, 11),//20
	SND(CURRENT, 35),	
#endif
};
#endif
#undef SND

static struct msm_snd_endpoints msm_device_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device msm_device_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_snd_endpoints
	},
};

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))

#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP))
//	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP)| \
//	(1<<MSM_ADSP_CODEC_MP3))

#define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP))
//	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP)| \
//	(1<<MSM_ADSP_CODEC_MP3))

#define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_WAV)|(1<<MSM_ADSP_CODEC_ADPCM)| \
	(1<<MSM_ADSP_CODEC_YADPCM)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DMA)), 0,
	0, 0, 0,

	/* Concurrency 1 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	 /* Concurrency 2 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 3 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 4 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 5 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 6 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 4),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 4),  /* AudPlay2BitStreamCtrlQueue */
//	DEC_INFO("AUDPLAY1TASK", 14, 1, 5),  /* AudPlay1BitStreamCtrlQueue */
//	DEC_INFO("AUDPLAY2TASK", 15, 2, 5),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 4),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	 * the only valid choice at this time. The board structure is
	 * set to all zeros by the C runtime initialization and that is now
	 * the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	 * include/linux/android_pmem.h.
	 */
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

#if 0
static struct android_pmem_platform_data android_pmem_gpu1_pdata = {
	.name = "pmem_gpu1",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};
#endif

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

// Description: Unable to play MP3 files, Create PMEM audio device
static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

#if 0
static struct platform_device android_pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 3,
	.dev = { .platform_data = &android_pmem_gpu1_pdata },
};
#endif

static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct platform_device msm_pmic_keyled = {
	.name = "pmic-led0",
};

#ifdef CONFIG_SWIFT_BATTERY_STUB
static struct platform_device lge_battery = {
	.name = "lge-battery",
};
#endif

static struct atcmd_platform_data eve_atcmd_pdata = {
	.name = "eve_atcmd",
};

static struct platform_device eve_atcmd_device = {
	.name = "eve_atcmd",
	.id = -1,
	.dev    = {
		.platform_data = &eve_atcmd_pdata
	},
}; 

#ifdef CONFIG_BATTERY_MSM
static u32 msm_calculate_batt_capacity(u32 current_voltage);
u32 calculate_capacity(u32 current_voltage);
u32 calculate_capacity(u32 current_voltage)
{
	int i;
	
	u32 cap=0;
	
	printk("%s: batt_vol=%d\n",__func__,current_voltage);
	for(i=0;i<101;i++){
		if(current_voltage<=3200){
			cap=0;
			
		}
		if(current_voltage>=3201 && current_voltage<=3210){
			cap=1;
 
		}

		if(current_voltage>=3211 && current_voltage<=3220){
			cap=2;
 
		}

		if(current_voltage>=3221 && current_voltage<=3230){
			cap=3;
 
		}

		if(current_voltage>=3231 && current_voltage<=3240){
			cap=4;
 
		}

		if(current_voltage>=3241 && current_voltage<=3250){
			cap=5;
 
		}

		if(current_voltage>=3251 && current_voltage<=3260){
			cap=6;
 
		}

		if(current_voltage>=3261 && current_voltage<=3270){
			cap=7;
 
		}

		if(current_voltage>=3271 && current_voltage<=3280){
			cap=8;
 
		}

		if(current_voltage>=3281 && current_voltage<=3290){
			cap=9;
 
		}

		if(current_voltage>=3291 && current_voltage<=3300){
			cap=10;
 
		}

		if(current_voltage>=3301 && current_voltage<=3310){
			cap=11;
 
		}

		if(current_voltage>=3311 && current_voltage<=3320){
			cap=12;
 
		}

		if(current_voltage>=3321 && current_voltage<=3330){
			cap=13;
 
		}

		if(current_voltage>=3331 && current_voltage<=3340){
			cap=14;
 
		}

		if(current_voltage>=3341 && current_voltage<=3350){
			cap=15;
 
		}

		if(current_voltage>=3351 && current_voltage<=3360){
			cap=16;
 
		}

		if(current_voltage>=3361 && current_voltage<=3370){
			cap=17;
 
		}

		if(current_voltage>=3371 && current_voltage<=3380){
			cap=18;
 
		}

		if(current_voltage>=3381 && current_voltage<=3390){
			cap=19;
 
		}

		if(current_voltage>=3391 && current_voltage<=3400){
			cap=20;
 
		}

		if(current_voltage>=3401 && current_voltage<=3410){
			cap=21;
 
		}

		if(current_voltage>=3411 && current_voltage<=3420){
			cap=22;
 
		}

		if(current_voltage>=3421 && current_voltage<=3430){
			cap=23;
 
		}

		if(current_voltage>=3431 && current_voltage<=3440){
			cap=24;
 
		}

		if(current_voltage>=3441 && current_voltage<=3450){
			cap=25;
 
		}

		if(current_voltage>=3451 && current_voltage<=3460){
			cap=26;
 
		}

		if(current_voltage>=3461 && current_voltage<=3470){
			cap=27;
 
		}

		if(current_voltage>=3471 && current_voltage<=3480){
			cap=28;
 
		}

		if(current_voltage>=3481 && current_voltage<=3490){
			cap=29;
 
		}

		if(current_voltage>=3491 && current_voltage<=3500){
			cap=30;
 
		}

		if(current_voltage>=3501 && current_voltage<=3510){
			cap=31;
 
		}

		if(current_voltage>=3511 && current_voltage<=3520){
			cap=32;
 
		}

		if(current_voltage>=3521 && current_voltage<=3530){
			cap=33;
 
		}

		if(current_voltage>=3531 && current_voltage<=3540){
			cap=34;
 
		}

		if(current_voltage>=3541 && current_voltage<=3550){
			cap=35;
 
		}

		if(current_voltage>=3551 && current_voltage<=3560){
			cap=36;
 
		}

		if(current_voltage>=3561 && current_voltage<=3470){
			cap=37;
 
		}

		if(current_voltage>=3571 && current_voltage<=3580){
			cap=38;
 
		}

		if(current_voltage>=3581 && current_voltage<=3590){
			cap=39;
 
		}

		if(current_voltage>=3591 && current_voltage<=3600){
			cap=40;
 
		}

		if(current_voltage>=3601 && current_voltage<=3610){
			cap=41;
 
		}

		if(current_voltage>=3611 && current_voltage<=3620){
			cap=42;
 
		}

		if(current_voltage>=3621 && current_voltage<=3630){
			cap=43;
 
		}

		if(current_voltage>=3631 && current_voltage<=3640){
			cap=44;
 
		}

		if(current_voltage>=3641 && current_voltage<=3650){
			cap=45;
 
		}

		if(current_voltage>=3651 && current_voltage<=3660){
			cap=46;
 
		}

		if(current_voltage>=3661 && current_voltage<=3670){
			cap=47;
 
		}

		if(current_voltage>=3671 && current_voltage<=3680){
			cap=48;
 
		}

		if(current_voltage>=3681 && current_voltage<=3690){
			cap=49;
 
		}

		if(current_voltage>=3691 && current_voltage<=3700){
			cap=50;
 
		}

		if(current_voltage>=3701 && current_voltage<=3710){
			cap=51;
 
		}

		if(current_voltage>=3711 && current_voltage<=3720){
			cap=52;
 
		}

		if(current_voltage>=3721 && current_voltage<=3730){
			cap=53;
 
		}

		if(current_voltage>=3731 && current_voltage<=3740){
			cap=54;
 
		}

		if(current_voltage>=3741 && current_voltage<=3750){
			cap=55;
 
		}

		if(current_voltage>=3751 && current_voltage<=3760){
			cap=56;
 
		}

		if(current_voltage>=3761 && current_voltage<=3770){
			cap=57;
 
		}

		if(current_voltage>=3771 && current_voltage<=3780){
			cap=58;
 
		}

		if(current_voltage>=3781 && current_voltage<=3790){
			cap=59;
 
		}

		if(current_voltage>=3791 && current_voltage<=3800){
			cap=60;
 
		}

		if(current_voltage>=3801 && current_voltage<=3810){
			cap=61;
 
		}

		if(current_voltage>=3811 && current_voltage<=3820){
			cap=62;
 
		}

		if(current_voltage>=3821 && current_voltage<=3830){
			cap=63;
 
		}

		if(current_voltage>=3831 && current_voltage<=3840){
			cap=64;
 
		}

		if(current_voltage>=3841 && current_voltage<=3850){
			cap=65;
 
		}

		if(current_voltage>=3851 && current_voltage<=3860){
			cap=66;
 
		}

		if(current_voltage>=3861 && current_voltage<=3870){
			cap=67;
 
		}

		if(current_voltage>=3871 && current_voltage<=3880){
			cap=68;
 
		}

		if(current_voltage>=3881 && current_voltage<=3890){
			cap=69;
 
		}

		if(current_voltage>=3891 && current_voltage<=3900){
			cap=70;
 
		}

		if(current_voltage>=3901 && current_voltage<=3910){
			cap=71;
 
		}

		if(current_voltage>=3911 && current_voltage<=3920){
			cap=72;
 
		}

		if(current_voltage>=3921 && current_voltage<=3930){
			cap=73;
 
		}

		if(current_voltage>=3931 && current_voltage<=3940){
			cap=74;
 
		}

		if(current_voltage>=3941 && current_voltage<=3950){
			cap=75;
 
		}

		if(current_voltage>=3951 && current_voltage<=3960){
			cap=76;
 
		}

		if(current_voltage>=3961 && current_voltage<=3970){
			cap=77;
 
		}

		if(current_voltage>=3971 && current_voltage<=3980){
			cap=78;
 
		}

		if(current_voltage>=3981 && current_voltage<=3990){
			cap=79;
 
		}

		if(current_voltage>=3991 && current_voltage<=4000){
			cap=80;
 
		}

		if(current_voltage>=4001 && current_voltage<=4010){
			cap=81;
 
		}

		if(current_voltage>=4011 && current_voltage<=4020){
			cap=82;
 
		}

		if(current_voltage>=4021 && current_voltage<=4030){
			cap=83;
 
		}

		if(current_voltage>=4031 && current_voltage<=4040){
			cap=84;
 
		}

		if(current_voltage>=4041 && current_voltage<=4050){
			cap=85;
 
		}

		if(current_voltage>=4051 && current_voltage<=4060){
			cap=86;
 
		}

		if(current_voltage>=4061 && current_voltage<=4070){
			cap=87;
 
		}

		if(current_voltage>=4071 && current_voltage<=4080){
			cap=88;
 
		}

		if(current_voltage>=4081 && current_voltage<=4090){
			cap=89;
 
		}

		if(current_voltage>=4091 && current_voltage<=4100){
			cap=90;
 
		}

		if(current_voltage>=4101 && current_voltage<=4110){
			cap=91;
 
		}

		if(current_voltage>=4111 && current_voltage<=4120){
			cap=92;
 
		}

		if(current_voltage>=4121 && current_voltage<=4130){
			cap=93;
 
		}

		if(current_voltage>=4131 && current_voltage<=4140){
			cap=94;
 
		}

		if(current_voltage>=4141 && current_voltage<=4150){
			cap=95;
 
		}

		if(current_voltage>=4151 && current_voltage<=4160){
			cap=96;
 
		}

		if(current_voltage>=4161 && current_voltage<=4170){
			cap=97;
 
		}

		if(current_voltage>=4171 && current_voltage<=4180){
			cap=98;
 
		}

		if(current_voltage>=4181 && current_voltage<=4190){
			cap=99;
 
		}

		if(current_voltage>=4191 && current_voltage<=4200){
			cap=100;
 
		}
	printk("%s: capacity=%d\n",__func__,cap);
	return cap;
 }
}

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 3200,
	.voltage_max_design	= 4300,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity	= &calculate_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	return calculate_capacity(current_voltage);
}

#endif 
static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

static struct platform_device hs_device = {
   .name = "msm-handset",
   .id   = -1,
   .dev  = {
      .platform_data = "7k_handset",
   },
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	int ret = -EPERM;
	if( machine_is_msm7x27_swift() ) {
		if (!strcmp(name, "lcdc_sharp_wvga"))
			ret = 0;
		else
			ret = -ENODEV;
	}
	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

#if 0
#ifdef CONFIG_BT
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};

enum {
	BT_WAKE,
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
	BT_PCM_DOUT,
	BT_PCM_DIN,
	BT_PCM_SYNC,
	BT_PCM_CLK,
	BT_HOST_WAKE,
};

#define GPIO_BT_RESET_N     96
#define GPIO_BT_REG_ON      21
#define GPIO_WL_RESET_N     CONFIG_BCM4325_GPIO_WL_RESET

static unsigned bt_config_power_on[] = {
	GPIO_CFG(42, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* WAKE */
	GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* HOST_WAKE */
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(42, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* WAKE */
	GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* RFR */
	GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* CTS */
	GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Rx */
	GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Tx */
	GPIO_CFG(68, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* HOST_WAKE */
};

static int bluetooth_power(int on)
{
	int pin, rc;

	printk(KERN_DEBUG "++%s, %d++\n", __func__, on);

	if (on) 
	{
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_on[pin], rc);
				return -EIO;
			}
		}

		/*Regulator On*/
		if(!gpio_get_value(GPIO_BT_REG_ON))
			gpio_set_value(GPIO_BT_REG_ON, 1);

		/*wait for regulator turned-on, 200ms*/
		msleep (200);		
		
		/*Reset Off*/
		gpio_set_value(GPIO_BT_RESET_N, 0);

		/*Waiting for chip*/
		msleep(150);/*BCM4325 Requirement*/

		/*Reset On*/
		gpio_set_value(GPIO_BT_RESET_N, 1);

		/*for safety*/
		msleep(20);
	} 
	else 
	{
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
			rc = gpio_tlmm_config(bt_config_power_off[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}

		gpio_set_value(GPIO_BT_RESET_N, 0);
		if(!gpio_get_value(GPIO_WL_RESET_N))
			gpio_set_value(GPIO_BT_REG_ON, 0);
	}

	//printk(KERN_DEBUG "--%s--(%d)(%d)\n", __func__, gpio_get_value(GPIO_BT_RESET_N), gpio_get_value(GPIO_BT_REG_ON));

	return 0;
}

static void __init bt_power_init(void)
{
	msm_bt_power_device.dev.platform_data = &bluetooth_power;

	gpio_request(GPIO_BT_RESET_N, "gpio_bt_reset_n");
	gpio_request(GPIO_BT_REG_ON, "gpio_bt_reg_on");
	gpio_configure(GPIO_BT_RESET_N, GPIOF_DRIVE_OUTPUT);
	gpio_configure(GPIO_BT_REG_ON, GPIOF_DRIVE_OUTPUT);
}
#else
#define bt_power_init(x) do {} while (0)
#endif
#endif

static struct resource kgsl_resources[] = {
	{
		.name = "kgsl_reg_memory",
		.start = 0xA0000000,
		.end = 0xA001ffff,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "kgsl_phys_memory",
		.start = 0,
		.end = 0,
		.flags = IORESOURCE_MEM,
	},
#if 1	
	{
		.name = "kgsl_yamato_irq",
		.start = INT_GRAPHICS,
		.end = INT_GRAPHICS,
		.flags = IORESOURCE_IRQ,
	},
#else	
	{
		.start = INT_GRAPHICS,
		.end = INT_GRAPHICS,
		.flags = IORESOURCE_IRQ,
	},
#endif
};

static struct kgsl_platform_data kgsl_pdata;

static struct platform_device msm_device_kgsl = {
	.name = "kgsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(kgsl_resources),
	.resource = kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};

#if 0 
static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds",
	.id = -1,
};
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define MSM7X27_EBI1_CS0_BASE      0x00200000
#define SWIFT_RAM_CONSOLE_BASE (MSM7X27_EBI1_CS0_BASE + 214 * SZ_1M)
#define SWIFT_RAM_CONSOLE_SIZE (128 * SZ_1K)
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource[] = {
   {
      .name = "ram_console",
      .start = SWIFT_RAM_CONSOLE_BASE,
      .end = SWIFT_RAM_CONSOLE_BASE + SWIFT_RAM_CONSOLE_SIZE - 1,
      .flags = IORESOURCE_MEM,
   }
};

static struct platform_device ram_console_device = {
   .name = "ram_console",
   .id = -1,
   .num_resources = ARRAY_SIZE(ram_console_resource),
   .resource = ram_console_resource,
};
#endif

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 83,
		.end	= 83,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 42,
		.end	= 42,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(83),
		.end	= MSM_GPIO_TO_INT(83),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

static struct i2c_board_info i2c_devices[] = {
#ifdef CONFIG_ISX005
	{
		I2C_BOARD_INFO("isx005", 0x1A),
	},
#endif	
};

static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
	GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
	GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
};

static uint32_t camera_on_gpio_table[] = {
   /* parallel CAMERA interfaces */
   GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
   GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
   GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
   GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
   GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
   GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
   GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
   GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
   GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* PCLK */
   GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
   GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
   GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA), /* MCLK */

};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

static struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.mdcphy = MSM_MDC_PHYS,
	.ioext.mdcsz  = MSM_MDC_SIZE,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};

#ifdef CONFIG_ISX005

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PMIC,
};

static struct msm_camera_sensor_flash_data flash_isx005 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_isx005_data = {
	.sensor_name    = "isx005",
	.sensor_reset   = 0,
	.sensor_pwd     = 1,
//	.vcm_pwd        = 0,
	.pdata          = &msm_camera_device_data,
	.flash_data     = &flash_isx005
};

static struct platform_device msm_camera_sensor_isx005 = {
	.name      = "msm_camera_isx005",
	.dev       = {
		.platform_data = &msm_camera_sensor_isx005_data,
	},
};
#endif

static struct platform_device ers_kernel = {
	.name = "ers-kernel",
};

static struct platform_device rt9393_bl = {
	.name = "rt9393-bl",
};

#if defined(CONFIG_FB_MSM_MDDI_SWIFT_PANEL)
static int mddi_ss_driveric_backlight_level(int level)
{
	/* TODO: return valid backlight level */
	return -1;
}

static struct msm_panel_common_pdata mddi_ss_driveric_pdata = {
/*	.backlight_level = mddi_ss_driveric_backlight_level,*/
};

struct platform_device mddi_ss_driveric_device = {
	.name   = "mddi_ss",
	.id     = 0,
	.dev    = {
		.platform_data = &mddi_ss_driveric_pdata,
	}
};
#else
struct platform_device mddi_ss_driveric_device = {
	.name   = "test_lcd",
	.id     = 0,
	.dev    = {
//		.platform_data = &mddi_ss_driveric_pdata,
	}
};
#endif

static struct platform_device *devices[] __initdata = {
#if !defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart3,
#endif
	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,
	
	&msm_device_otg,
	&msm_device_hsusb_otg,
	&msm_device_hsusb_host,
	&msm_device_hsusb_peripheral,
	&msm_device_gadget_peripheral,
#ifdef CONFIG_USB_FUNCTION
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_ANDROID
	&android_usb_device,
#endif
	&msm_device_i2c,
	&msm_device_tssc,
	&android_pmem_kernel_ebi1_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
// Description: Unable to play MP3 files, Create PMEM audio device
	&android_pmem_audio_device, 
#if 0
	&android_pmem_gpu1_device,
#endif

	&msm_fb_device,
	&msm_device_uart_dm1,
#ifdef CONFIG_BT
#if 0
	 &msm_bt_power_device,
#endif	 
#endif
    /* TODO : snd device should be added, cleaneye */
#if 0
	&msm_device_pmic_leds,
#endif
    &msm_device_snd,
	&msm_device_adspdec,
    /* TODO : blue sleep should be added, cleaneye */
#if 0
	&msm_bluesleep_device,
#endif	
	&msm_device_kgsl,
#if 1 /* msm-handsetn platform deivce reigistration, kenobi */
   &hs_device,
#endif
    /* add swift specific devices at following lines*/
	&msm_pmic_keyled,

#ifdef CONFIG_SWIFT_BATTERY_STUB
	&lge_battery,
#endif

#ifdef CONFIG_BATTERY_MSM
	&msm_batt_device,
#endif
#ifdef CONFIG_ANDROID_RAM_CONSOLE
   &ram_console_device,
#endif

#ifdef CONFIG_ISX005
	&msm_camera_sensor_isx005,
#endif
	&ers_kernel,
	&rt9393_bl,
	&mddi_ss_driveric_device,	
	&eve_atcmd_device, //vlc	
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
};

#if 0
static int mddi_power_save_on;
static void msm_fb_mddi_power_save(int on)
{
	int flag_on = !!on;
	int ret;


	if (mddi_power_save_on == flag_on)
		return;

	mddi_power_save_on = flag_on;

	if (!flag_on && machine_is_qsd8x50_ffa()) {
		gpio_set_value(MDDI_RST_OUT_GPIO, 0);
		mdelay(1);
	}

	ret = pmic_lp_mode_control(flag_on ? OFF_CMD : ON_CMD,
		PM_VREG_LP_MSME2_ID);
	if (ret)
		printk(KERN_ERR "%s: pmic_lp_mode_control failed!\n", __func__);

	msm_fb_vreg_config("gp5", flag_on);
	msm_fb_vreg_config("boost", flag_on);

	if (flag_on && machine_is_qsd8x50_ffa()) {
		gpio_set_value(MDDI_RST_OUT_GPIO, 0);
		mdelay(1);
		gpio_set_value(MDDI_RST_OUT_GPIO, 1);
		gpio_set_value(MDDI_RST_OUT_GPIO, 1);
		mdelay(1);
	}
}

static struct mddi_platform_data mddi_pdata = {
        .mddi_power_save = msm_fb_mddi_power_save,
};
#endif
static struct mddi_platform_data mddi_pdata = {
 //       .mddi_power_save = msm_fb_mddi_power_save,
};

static void __init msm_fb_add_devices(void)
{
        msm_fb_register_device("mdp", 0);
//        msm_fb_register_device("ebi2", 0);
        msm_fb_register_device("pmdh", &mddi_pdata);
//        msm_fb_register_device("emdh", 0);
        //msm_fb_register_device("tvenc", &tvenc_pdata);
}

extern struct sys_timer msm_timer;

static void __init msm7x27_init_irq(void)
{
	msm_init_irq();
}

static struct msm_acpu_clock_platform_data msm7x27_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 256000,
	.vdd_switch_time_us = 62,
	.power_collapse_khz = 19200000,
	.wait_for_irq_khz = 128000000,
	.max_axi_khz = 128000,
};

void msm_serial_debug_init(unsigned int base, int irq,
			   struct device *clk_device, int signal_irq);

static void sdcc_gpio_init(void)
{
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	int rc = 0;
	if (gpio_request(GPIO_MMC_CD_N, "sdc2_status_irq"))
		pr_err("failed to request gpio sdc2_status_irq\n");

#if 0
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_MMC_CD_N, 0, GPIO_INPUT, GPIO_PULL_UP,
					GPIO_2MA), GPIO_ENABLE);
#else
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_MMC_CD_N, 0, GPIO_INPUT, GPIO_NO_PULL,
				GPIO_2MA), GPIO_ENABLE);
#endif

	if (rc)
		printk(KERN_ERR "%s: Failed to configure GPIO %d\n",
				__func__, rc);
#if defined(CONFIG_MACH_MSM7X27_SWIFT_REV_1)
	
	if (gpio_request(GPIO_MMC_CD_COVER, "sdc2_status_irq"))
		pr_err("failed to request gpio sdc2_status_irq\n");
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_MMC_CD_COVER, 0, GPIO_INPUT, GPIO_NO_PULL,GPIO_2MA), GPIO_ENABLE);

	if (rc)
		printk(KERN_ERR "%s: Failed to configure GPIO %d\n",__func__, rc);
#endif
												
#endif

	/* SDC1 GPIOs */
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	if (gpio_request(51, "sdc1_data_3"))
		pr_err("failed to request gpio sdc1_data_3\n");
	if (gpio_request(52, "sdc1_data_2"))
		pr_err("failed to request gpio sdc1_data_2\n");
	if (gpio_request(53, "sdc1_data_1"))
		pr_err("failed to request gpio sdc1_data_1\n");
	if (gpio_request(54, "sdc1_data_0"))
		pr_err("failed to request gpio sdc1_data_0\n");
	if (gpio_request(55, "sdc1_cmd"))
		pr_err("failed to request gpio sdc1_cmd\n");
	if (gpio_request(56, "sdc1_clk"))
		pr_err("failed to request gpio sdc1_clk\n");
#endif

	if (machine_is_msm7x27_ffa() || machine_is_msm7x27_ffa())
		return;

	/* SDC2 GPIOs */
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (gpio_request(62, "sdc2_clk"))
		pr_err("failed to request gpio sdc2_clk\n");
	if (gpio_request(63, "sdc2_cmd"))
		pr_err("failed to request gpio sdc2_cmd\n");
	if (gpio_request(64, "sdc2_data_3"))
		pr_err("failed to request gpio sdc2_data_3\n");
	if (gpio_request(65, "sdc2_data_2"))
		pr_err("failed to request gpio sdc2_data_2\n");
	if (gpio_request(66, "sdc2_data_1"))
		pr_err("failed to request gpio sdc2_data_1\n");
	if (gpio_request(67, "sdc2_data_0"))
		pr_err("failed to request gpio sdc2_data_0\n");
#endif

	/* SDC3 GPIOs */
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	if (gpio_request(88, "sdc3_clk"))
		pr_err("failed to request gpio sdc3_clk\n");
	if (gpio_request(89, "sdc3_cmd"))
		pr_err("failed to request gpio sdc3_cmd\n");
	if (gpio_request(90, "sdc3_data_3"))
		pr_err("failed to request gpio sdc3_data_3\n");
	if (gpio_request(91, "sdc3_data_2"))
		pr_err("failed to request gpio sdc3_data_2\n");
	if (gpio_request(92, "sdc3_data_1"))
		pr_err("failed to request gpio sdc3_data_1\n");
	if (gpio_request(93, "sdc3_data_0"))
		pr_err("failed to request gpio sdc3_data_0\n");
#endif

	/* SDC4 GPIOs */
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	if (gpio_request(19, "sdc4_data_3"))
		pr_err("failed to request gpio sdc4_data_3\n");
	if (gpio_request(20, "sdc4_data_2"))
		pr_err("failed to request gpio sdc4_data_2\n");
	if (gpio_request(21, "sdc4_data_1"))
		pr_err("failed to request gpio sdc4_data_1\n");
	if (gpio_request(107, "sdc4_cmd"))
		pr_err("failed to request gpio sdc4_cmd\n");
	if (gpio_request(108, "sdc4_data_0"))
		pr_err("failed to request gpio sdc4_data_0\n");
	if (gpio_request(109, "sdc4_clk"))
		pr_err("failed to request gpio sdc4_clk\n");
#endif
}

static unsigned sdcc_cfg_data[][6] = {
	/* SDC1 configs */
	{
	GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	},
	/* SDC2 configs */
	{
	GPIO_CFG(62, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	GPIO_CFG(63, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(64, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(65, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(66, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(67, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	},
	/* SDC3 configs */
	{
	GPIO_CFG(88, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	GPIO_CFG(89, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(90, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(91, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(92, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(93, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	},
	/* SDC4 configs */
	{
	GPIO_CFG(19, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(20, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(21, 4, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(107, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(108, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(109, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	}
};

static unsigned long vreg_sts, gpio_sts;
static unsigned mpp_mmc = 2;
//static struct mpp *mpp_mmc;
static struct vreg *vreg_mmc;

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int i, rc;

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable)
		set_bit(dev_id, &gpio_sts);
	else
		clear_bit(dev_id, &gpio_sts);

	for (i = 0; i < ARRAY_SIZE(sdcc_cfg_data[dev_id - 1]); i++) {
		rc = gpio_tlmm_config(sdcc_cfg_data[dev_id - 1][i],
			enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc)
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, sdcc_cfg_data[dev_id - 1][i], rc);
	}
}

struct mmc_vdd_xlat {
	int mask;
	int level;
};

#if 0
static struct mmc_vdd_xlat mmc_vdd_table[] = {
	{ MMC_VDD_165_195,	1800 },
	{ MMC_VDD_20_21,	2050 },
	{ MMC_VDD_21_22,	2150 },
	{ MMC_VDD_22_23,	2250 },
	{ MMC_VDD_23_24,	2350 },
	{ MMC_VDD_24_25,	2450 },
	{ MMC_VDD_25_26,	2550 },
	{ MMC_VDD_26_27,	2650 },
	{ MMC_VDD_27_28,	2650 },
	{ MMC_VDD_28_29,	2650 },
	{ MMC_VDD_29_30,	2650 },
};

static unsigned int current_vdd =0;
#endif


static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;
	int i;
	unsigned cnt = 0;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	printk(KERN_ERR "(%s)vdd :%x\n", __func__, vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
			if (machine_is_msm7x27_ffa()) {
				rc = mpp_config_digital_out(mpp_mmc,
				     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     MPP_DLOGIC_OUT_CTRL_LOW));
			} else {
#if 1
				rc = vreg_set_level(vreg_mmc, 0);
				cnt = vreg_get_refcnt(vreg_mmc);
				for(i = 0; i < cnt; i++)
					rc = vreg_disable(vreg_mmc);
				printk(KERN_DEBUG "%s: Refcnt %d\n", 
						__func__, vreg_get_refcnt(vreg_mmc));
#else
				rc = vreg_set_level(vreg_mmc, 0);
				vreg_disable(vreg_mmc);
#endif
			}
			if (rc)
				printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
		}
		return 0;
	}
#if 0 
	else	{
		if(current_vdd != vdd) {
			current_vdd = vdd;
			for (i = 0; i < ARRAY_SIZE(mmc_vdd_table); i++) {
				if (mmc_vdd_table[i].mask == (1 << vdd)) {
					printk(KERN_DEBUG "%s: Setting level to %u\n",__func__, mmc_vdd_table[i].level);
					rc = vreg_set_level(vreg_mmc, mmc_vdd_table[i].level);
					if (rc) {
					printk(KERN_ERR
					       "%s: Error setting vreg level (%d)\n",
					       __func__, rc);
					}
				}
			}
		}else {
			printk(KERN_DEBUG "%s: Current Vdd : %x Vdd :%x\n",__func__, current_vdd, vdd);
		}
	}
#endif 
	if (!vreg_sts) {
		if (machine_is_msm7x27_ffa()) {
			rc = mpp_config_digital_out(mpp_mmc,
			     MPP_CFG(MPP_DLOGIC_LVL_MSMP,
			     MPP_DLOGIC_OUT_CTRL_HIGH));
		} else {
#if 1 
			rc = vreg_set_level(vreg_mmc, 2650);
			if (!rc) {
				rc = vreg_enable(vreg_mmc);
			}
		}
#else
			rc = vreg_enable(vreg_mmc);
			if (rc) {
				printk(KERN_ERR "%s: Error enabling vreg (%d)\n",
						   __func__, rc);
			}
		}
#endif 
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	set_bit(pdev->id, &vreg_sts);
	return 0;
}

#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int swift_sdcc_slot_status(struct device *dev)
{
#if defined(CONFIG_MACH_MSM7X27_SWIFT_REV_1)
	return !(gpio_get_value(GPIO_MMC_CD_N) || gpio_get_value(GPIO_MMC_CD_COVER));
#else
	return !gpio_get_value(GPIO_MMC_CD_N);
#endif
}
#endif

static struct mmc_platform_data msm7x27_sdcc_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
};

#ifdef CONFIG_LGE_BCM432X_PATCH
static unsigned int bcm432x_sdcc_wlan_slot_status(struct device *dev)
{
	return gpio_get_value(CONFIG_BCM4325_GPIO_WL_RESET);
}

#if 1
static struct mmc_platform_data bcm432x_sdcc_wlan_data = {
    .ocr_mask   	= MMC_VDD_30_31,
	.translate_vdd	= msm_sdcc_setup_power,
    .status     	= bcm432x_sdcc_wlan_slot_status,
#if 1 //sungwoo LMH_WIFI 2010-05-04 to change wlan_driver
	.status_irq 	= MSM_GPIO_TO_INT(CONFIG_BCM4325_GPIO_WL_RESET),
#else
	.status_irq		= MSM_GPIO_TO_INT(CONFIG_BCM4329_GPIO_WL_RESET),
#endif
    .irq_flags      = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
    .mmc_bus_width  = MMC_CAP_4_BIT_DATA,
};
#else
static struct mmc_platform_data bcm432x_sdcc_wlan_data = {
	.ocr_mask	= MMC_VDD_30_31,
	.translate_vdd	= msm_sdcc_setup_power,
	.status		= bcm432x_sdcc_wlan_slot_status,
	.irq_flags      = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
};
#endif
#endif /* CONFIG_LGE_BCM432X_PATCH*/

#define SWIFT_MMC_VDD (MMC_VDD_165_195 | MMC_VDD_20_21 | MMC_VDD_21_22 \
			| MMC_VDD_22_23 | MMC_VDD_23_24 | MMC_VDD_24_25 \
			| MMC_VDD_25_26 | MMC_VDD_26_27 | MMC_VDD_27_28 \
			| MMC_VDD_28_29 | MMC_VDD_29_30)

#if 1
static struct mmc_platform_data msm7x2x_sdcc_data = {
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION

// SUNGWOO_EDBAE SWIFT MMC MODIFIED [
#if 0
	.ocr_mask		= MMC_VDD_30_31,
#else
	.ocr_mask		= SWIFT_MMC_VDD,
#endif
// SUNGWOO_EDBAE SWIFT MMC MODIFIED ]

	.translate_vdd	= msm_sdcc_setup_power,
	.status 		= swift_sdcc_slot_status, 
// SUNGWOO_EDBAE SWIFT MMC MODIFIED [	
#if 0	
	.status_irq 	= MSM_GPIO_TO_INT(GPIO_SD_DETECT_N),
#else	
	.status_irq 	= MSM_GPIO_TO_INT(GPIO_MMC_CD_COVER),	
#endif
// SUNGWOO_EDBAE SWIFT MMC MODIFIED ]
	.irq_flags		= IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.mmc_bus_width	= MMC_CAP_4_BIT_DATA,
#else
	.ocr_mask		= MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif	
};
#else
static struct mmc_platform_data swift_sdcc_data = {
#ifdef CONFIG_MACH_MSM7X27_SWIFT
//	.ocr_mask   = MMC_VDD_28_29,
	.ocr_mask	= SWIFT_MMC_VDD,
#else
	.ocr_mask   = MMC_VDD_30_31,
#endif
	.translate_vdd  = msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status         = swift_sdcc_slot_status,
//	.status_irq = MSM_GPIO_TO_INT(GPIO_MMC_CD_N),
//	.irq_flags      = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
};
#endif


static void __init msm7x27_init_mmc(void)
#if 1
{
	if (!machine_is_msm7x25_ffa() && !machine_is_msm7x27_ffa()) {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	}

	sdcc_gpio_init();
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	msm_add_sdcc(1, &msm7x2x_sdcc_data);
#endif	
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
#if defined(CONFIG_LGE_BCM432X_PATCH)
#if 1 //sungwoo LMH_WIFI 2010-05-04 to change wlan_driver
	gpio_request(CONFIG_BCM4325_GPIO_WL_RESET, "wlan_cd");
#else
    gpio_request(CONFIG_BCM4329_GPIO_WL_RESET, "wlan_cd");
#endif
//    gpio_direction_output(CONFIG_BCM4329_GPIO_WL_RESET, 0);
//    mdelay(150);
#if 1 //sungwoo LMH_WIFI 2010-05-04 to change wlan_driver
	gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4325_GPIO_WL_HOSTWAKEUP, 0, GPIO_OUTPUT, GPIO_PULL_UP,
							  GPIO_2MA), GPIO_ENABLE);
#else
	gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4329_GPIO_WL_HOSTWAKEUP, 0, GPIO_OUTPUT, GPIO_PULL_UP,
							  GPIO_2MA), GPIO_ENABLE);
#endif

#if 1   //SUNGWOO SWIFTMR_LMH_WIFI 2010-04-19
#if 1 //sungwoo LMH_WIFI 2010-05-04 to change wlan_driver
		gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4325_GPIO_WL_REGON, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_configure(CONFIG_BCM4325_GPIO_WL_REGON, GPIOF_DRIVE_OUTPUT);

		gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4325_GPIO_WL_RESET, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_configure(CONFIG_BCM4325_GPIO_WL_RESET, GPIOF_DRIVE_OUTPUT);

#else
		gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4329_GPIO_WL_REGON, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_configure(CONFIG_BCM4329_GPIO_WL_REGON, GPIOF_DRIVE_OUTPUT);

		gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4329_GPIO_WL_RESET, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_configure(CONFIG_BCM4329_GPIO_WL_RESET, GPIOF_DRIVE_OUTPUT);

#endif		
#endif

    msm_add_sdcc(2, &bcm432x_sdcc_wlan_data);
#else /* qualcomm or google */
    msm_add_sdcc(2, &msm7x2x_sdcc_data);
#endif
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	msm_add_sdcc(3, &msm7x2x_sdcc_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	msm_add_sdcc(4, &msm7x2x_sdcc_data);
#endif
}
#else
{
#if 1
	if (!machine_is_msm7x25_ffa() && !machine_is_msm7x27_ffa()) {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	}
#else	
	if (machine_is_msm7x27_ffa()) {
		mpp_mmc = mpp_get(NULL, "mpp3");
		if (!mpp_mmc) {
			printk(KERN_ERR "%s: mpp get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	} else if (machine_is_msm7x27_swift()) {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
					__func__, PTR_ERR(vreg_mmc));
			return;
		}
	} else {
		vreg_mmc = vreg_get(NULL, "mmc");
		if (IS_ERR(vreg_mmc)) {
			printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			       __func__, PTR_ERR(vreg_mmc));
			return;
		}
	}
#endif
	sdcc_gpio_init();

#if 1 // 4715
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
#if defined(CONFIG_MACH_MSM7X27_SWIFT_REV_1)
	msm_add_sdcc(1, &swift_sdcc_data, gpio_to_irq(GPIO_MMC_CD_COVER), IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING);
#else
	msm_add_sdcc(1, &swift_sdcc_data, gpio_to_irq(GPIO_MMC_CD_N), IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING);
#endif

#endif

	if (machine_is_msm7x27_surf()) {
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
		msm_add_sdcc(2, &msm7x27_sdcc_data, 0, 0);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
		msm_add_sdcc(3, &msm7x27_sdcc_data, 0, 0);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
		msm_add_sdcc(4, &msm7x27_sdcc_data, 0, 0);
#endif
	} else if (machine_is_msm7x27_swift()) {
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
#ifdef CONFIG_LGE_BCM432X_PATCH
		/* control output */
#ifdef CONFIG_BCM4325_GPIO_WL_REGON
		gpio_direction_output(CONFIG_BCM4325_GPIO_WL_REGON, 0);
#endif /* CONFIG_BCM4325_GPIO_WL_REGON */
	    printk(KERN_ERR "%s: machine_is_msm7x27_surf_swift [mingi]\n",
					__func__);
		gpio_direction_output(CONFIG_BCM4325_GPIO_WL_RESET, 0);
		mdelay(150);

/*		gpio_request(CONFIG_BCM4325_GPIO_WL_RESET, "wlan_cd");
		bcm432x_sdcc_wlan_data.status_irq =
	    	gpio_to_irq(CONFIG_BCM4325_GPIO_WL_RESET);	*/

/*		printk(KERN_ERR "[rypark] wlan_status_irq:%d\n", 
				bcm432x_sdcc_wlan_data.status_irq);	*/
		msm_add_sdcc(2, &bcm432x_sdcc_wlan_data, gpio_to_irq(CONFIG_BCM4325_GPIO_WL_RESET), IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING); // by mingi from dpkim's mail
#else /* CONFIG_LGE_BCM432X_PATCH */
		msm_add_sdcc(2, &swift_sdcc_data, 0, 0);
#endif /* CONFIG_LGE_BCM432X_PATCH */
#endif
#else // 4615
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	msm_add_sdcc(1, &msm7x27_sdcc_data);
#endif

	if (machine_is_msm7x27_surf()) {
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
		msm_add_sdcc(2, &msm7x27_sdcc_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
		msm_add_sdcc(3, &msm7x27_sdcc_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
		msm_add_sdcc(4, &msm7x27_sdcc_data);
#endif
	} else if (machine_is_msm7x27_swift()) {
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
		msm_add_sdcc(2, &swift_sdcc_data);
#endif

#endif
	}
}
#endif


static struct msm_pm_platform_data msm7x27_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 16000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 20000,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 12000,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 20000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 2000,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 0,
};

static void
msm_i2c_gpio_config(int iface, int config_type)
{
	int gpio_scl;
	int gpio_sda;
	if (iface) {
		gpio_scl = 95;
		gpio_sda = 96;
	} else {
		gpio_scl = 60;
		gpio_sda = 61;
	}
	if (config_type) {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	} else {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 400000,
	.rmutex = NULL,
	.pri_clk = 60,
	.pri_dat = 61,
#ifndef CONFIG_MACH_MSM7X27_SWIFT
	.aux_clk = 95,
	.aux_dat = 96,
#endif
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (gpio_request(60, "i2c_pri_clk"))
		pr_err("failed to request gpio i2c_pri_clk\n");
	if (gpio_request(61, "i2c_pri_dat"))
		pr_err("failed to request gpio i2c_pri_dat\n");
#ifndef CONFIG_MACH_MSM7X27_SWIFT
	if (gpio_request(95, "i2c_sec_clk"))
		pr_err("failed to request gpio i2c_sec_clk\n");
	if (gpio_request(96, "i2c_sec_dat"))
		pr_err("failed to request gpio i2c_sec_dat\n");
#endif
	msm_i2c_pdata.pm_lat =
		msm7x27_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN]
		.latency;
	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static void swift_init_pmic(void) 
{
	static struct vreg *vreg_aux2;
	int rc = 0;

	vreg_aux2 = vreg_get(NULL, "gp5");
	if (IS_ERR(vreg_aux2)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_aux2));
		return;
	}

	rc = vreg_set_level(vreg_aux2, 0);
	if (!rc)
		rc = vreg_enable(vreg_mmc);
	if (rc)
		printk(KERN_ERR "%s: return val: %d \n",
				__func__, rc);

	return;
}

#if 0
static unsigned msm_uart_csr_code[] = {
	0x00,		/* 	300 bits per second	*/
	0x11,		/* 	600 bits per second	*/
	0x22,		/* 	1200 bits per second	*/
	0x33,		/* 	2400 bits per second	*/
	0x44,		/* 	4800 bits per second	*/
	0x55,		/* 	9600 bits per second	*/
	0x66,		/* 	14.4K bits per second	*/
	0x77,		/* 	19.2K bits per second	*/
	0x88,		/* 	28.8K bits per second	*/
	0x99,		/* 	38.4K bits per second	*/
	0xAA,		/* 	57.6K bits per second	*/
	0xCC,		/* 	115.2K bits per second	*/
};

static struct msm_serial_platform_data msm_serial_pdata = {
	.uart_csr_code = msm_uart_csr_code,
};
#endif

static void __init msm7x27_init(void)
{
	if (socinfo_init() < 0)
		BUG();

	printk("SWIFT BOARD is %s\n", swift_rev[system_rev]);

#if 0
	msm_device_uart3.dev.platform_data = &msm_serial_pdata;
#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	msm_serial_debug_init(MSM_UART3_PHYS, INT_UART3,
			&msm_device_uart3.dev, 1);
#endif
#endif

	if (cpu_is_msm7x27())
		msm7x27_clock_data.max_axi_khz = 200000;

	msm_acpu_clock_init(&msm7x27_clock_data);


	/* Initialize the zero page for barriers and cache ops */
	map_zero_page_strongly_ordered();

	//msm_hsusb_pdata.max_axi_khz = msm7x27_clock_data.max_axi_khz;

	/* This value has been set to 160000 for power savings. */
	/* OEMs may modify the value at their discretion for performance */
	/* The appropriate maximum replacement for 160000 is: */
	/* clk_get_max_axi_khz() */
	kgsl_pdata.max_axi_freq = 160000;

	msm_hsusb_pdata.swfi_latency =
		msm7x27_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
	msm_device_hsusb_host.dev.platform_data = &msm_hsusb_pdata;
	platform_add_devices(devices, ARRAY_SIZE(devices));
//	msm_camera_add_device();
	msm_device_i2c_init();
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));

#ifdef CONFIG_SURF_FFA_GPIO_KEYPAD
	if (machine_is_msm7x27_ffa())
		platform_device_register(&keypad_device_7k_ffa);
	else
		platform_device_register(&keypad_device_surf);
#endif
	if (machine_is_msm7x27_swift()) 
		platform_device_register(&keypad_device_swift);
	
	msm_fb_add_devices();
	msm7x27_init_mmc();
#if 0
	bt_power_init();
#endif

	swift_init_pmic();
	
	if (cpu_is_msm7x27())
		msm_pm_set_platform_data(msm7x27_pm_data);
	else
		msm_pm_set_platform_data(msm7x27_pm_data);

	swift_add_btpower_devices();

	/* initialize swift specific devices */
	swift_init_gpio_i2c_devices();

	/* initialize timed_output vibrator */
	msm_init_timed_vibrator();
}

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static void __init pmem_mdp_size_setup(char **p)
{
	pmem_mdp_size = memparse(*p, p);
}
__early_param("pmem_mdp_size=", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static void __init pmem_audio_size_setup(char **p)
{
	pmem_audio_size = memparse(*p, p);
}
__early_param("pmem_audio_size=", pmem_audio_size_setup);


#if 0
static unsigned pmem_gpu1_size = MSM_PMEM_GPU1_SIZE;
static void __init pmem_gpu1_size_setup(char **p)
{
	pmem_gpu1_size = memparse(*p, p);
}
__early_param("pmem_gpu1_size=", pmem_gpu1_size_setup);
#endif

static unsigned fb_size = MSM_FB_SIZE;
static void __init fb_size_setup(char **p)
{
	fb_size = memparse(*p, p);
}
__early_param("fb_size=", fb_size_setup);

static unsigned gpu_phys_size = MSM_GPU_PHYS_SIZE;
static void __init gpu_phys_size_setup(char **p)
{
	gpu_phys_size = memparse(*p, p);
}
__early_param("gpu_phys_size=", gpu_phys_size_setup);

static void __init msm_msm7x27_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_mdp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for mdp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_audio_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	}
#if 0
	size = pmem_gpu1_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_gpu1_pdata.start = __pa(addr);
		android_pmem_gpu1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for gpu1 "
			"pmem arena\n", size, addr, __pa(addr));
	}
#endif
	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));
	
	g_fb_addr = __pa(addr);
	size = gpu_phys_size ? : MSM_GPU_PHYS_SIZE;
	addr = alloc_bootmem(size);
	kgsl_resources[1].start = __pa(addr);
	kgsl_resources[1].end = kgsl_resources[1].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for KGSL\n",
		size, addr, __pa(addr));
}

static void __init msm7x27_map_io(void)
{
	msm_map_common_io();
	
	msm_map_swift_io();
	/* Technically dependent on the SoC but using machine_is
	 * macros since socinfo is not available this early and there
	 * are plans to restructure the code which will eliminate the
	 * need for socinfo.
	 */
	if (machine_is_msm7x27_surf() || machine_is_msm7x27_ffa())
		msm_clock_init(msm_clocks_7x27, msm_num_clocks_7x27);
	else if (machine_is_msm7x27_swift())
		msm_clock_init(msm_clocks_7x27, msm_num_clocks_7x27);
	else
		msm_clock_init(msm_clocks_7x27, msm_num_clocks_7x27);
	msm_msm7x27_allocate_memory_regions();

#ifdef CONFIG_CACHE_L2X0
	if (/*machine_is_msm7x27_griffin() || */machine_is_msm7x27_surf() || 
			machine_is_msm7x27_ffa() || machine_is_msm7x27_swift()) {
		/* 7x27 has 256KB L2 cache:
			64Kb/Way and 4-Way Associativity;
			R/W latency: 3 cycles;
			evmon/parity/share disabled. */
		l2x0_init(MSM_L2CC_BASE, 0x00068012, 0xfe000000);
	}
#endif
}

static void __init swift_fixup(struct machine_desc *desc, struct tag *tags,
		char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].node = PHYS_TO_NID(PHYS_OFFSET);
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	mi->bank[0].size = (214*1024*1024);
#else
	mi->bank[0].size = (215*1024*1024);
#endif
}


MACHINE_START(MSM7X27_SWIFT, "Swift board(LGE GT540)")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io        = MSM_DEBUG_UART_PHYS,
	.io_pg_offst    = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params    = 0x00200100,
	.fixup      	= swift_fixup,
	.map_io         = msm7x27_map_io,
	.init_irq       = msm7x27_init_irq,
	.init_machine   = msm7x27_init,
	.timer          = &msm_timer,
MACHINE_END
