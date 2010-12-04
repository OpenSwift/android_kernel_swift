/* arch/arm/mach-msm/board-lge-measure-perf.h
 *
 * Copyright (C) 2009 LGE, Inc
 * Author: Jun-Yeong Han <j.y.han@lge.com>
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

#ifndef __BOARD_LGE_MEASURE_PERF_H__
#define __BOARD_LGE_MEASURE_PERF_H__

#define CYCLES_PER_SECOND	598 * 1000 * 1000

static inline unsigned rd_cycle_count(void) 
{
	unsigned cc;
	asm volatile (
		"mrc p15, 0, %0, c15, c12, 1\n" :
		"=r" (cc) 
	);
	return cc;
}

static inline void perf_enable(void)
{
	asm volatile (
		"mcr p15, 0, %0, c15, c12, 0\n" : : "r" (7) 
	);
}

static inline void perf_disable(void)
{
	asm volatile (
		"mcr p15, 0, %0, c15, c12, 0\n" : : "r" (0)
	);
}

#define BUFFER_MAX	1024

#define RUN_TIME_MEASURE_START									\
	char output_buffer[BUFFER_MAX];								\
	unsigned cycle_count;									\
	sprintf(output_buffer, "From [%s]%s:%d", __FILE__, __FUNCTION__, __LINE__);		\
	perf_enable();										\
	cycle_count = rd_cycle_count();

#define RUN_TIME_MEASURE_END									\
	cycle_count = rd_cycle_count() - cycle_count;						\
	perf_disable();										\
	printk("%s\n", output_buffer);								\
	printk("To [%s]%s:%d\n", __FILE__, __FUNCTION__, __LINE__);				\
	printk("Run Time = %d/%d(seconds)\n", cycle_count, CYCLES_PER_SECOND);

#endif

