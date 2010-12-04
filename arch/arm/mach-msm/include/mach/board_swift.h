/* arch/arm/mach-msm/include/mach/board_griffin.h
 * Copyright (C) 2009 LGE, Inc.
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
 */
#ifndef __ASM_ARCH_MSM_BOARD_SWIFT_H
#define __ASM_ARCH_MSM_BOARD_SWIFT_H

#include <linux/types.h>
#include <linux/list.h>
#include <asm/setup.h>

/* board revision information */
enum {
	SWIFT_REV_A = 0,
	SWIFT_REV_B,
	SWIFT_REV_C,
	SWIFT_REV_D,
	SWIFT_REV_E,
	SWIFT_REV_TOT_NUM,
};

static char *swift_rev[] = {
	"Rev.A",
	"Rev.B",
	"Rev.C",
	"Rev.D",
	"Rev.E",	
};

/* gpio-i2c related functions */
void swift_init_gpio_i2c_devices(void);

void __init msm_map_swift_io(void);

extern struct platform_device keypad_device_swift;

struct bluetooth_platform_data {
	int (*bluetooth_power)(int on);
	int (*bluetooth_toggle_radio)(void *data, enum rfkill_state state);
};

struct bluesleep_platform_data {
	int bluetooth_port_num;
};

void __init swift_add_btpower_devices(void);


#endif
