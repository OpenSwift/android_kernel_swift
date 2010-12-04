/* arch/arm/mach-msm/board-griffin-ers.c
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

#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/limits.h>

#define ERS_DRIVER_NAME "ers-kernel"

static atomic_t enable = ATOMIC_INIT(0);
static atomic_t report_num = ATOMIC_INIT(1);

struct ram_console_buffer {
	uint32_t    sig;
	uint32_t    start;
	uint32_t    size;
	uint8_t     data[0];
};

static struct ram_console_buffer *ram_console_buffer = 0;
extern struct ram_console_buffer *get_ram_console_buffer(void);

int get_lock_dependencies_report_start(uint32_t start, uint32_t size, uint8_t *data)
{
	int report_start;
	int i;	

	report_start = -1;

	for (i = start - 1; i > -1; --i) {
		if (data[i] == '-') {
			if (!strncmp(&data[i - 25], "-lock_dependencies_report-", 26)) {
				report_start = i + 1;
				break;
			}
		}
	}

	if (i > -1) {
		return report_start;		
	}

	for (i = size - 1; i >= start; --i) {
		if (data[i] == '-') {
			if (!strncmp(&data[i - 25], "-lock_dependencies_report-", 26)) {
				report_start = i + 1;
				break;
			}
		}
	}

	if (i < start) {
		return -1;
	}

	return report_start;
}

void lock_dependencies_report(void)
{
	char filename[NAME_MAX];
	int fd;
	int value;
	mm_segment_t oldfs;

	uint32_t start;
	uint32_t size;
	uint8_t *data;
	int report_start;

	value = atomic_read(&enable);
	if (value == 0) {
		return;
	}

	ram_console_buffer = get_ram_console_buffer();
	if (!ram_console_buffer) {
		return;
	}
	
	start = ram_console_buffer->start; 
	size = ram_console_buffer->size;
	data = ram_console_buffer->data;
	
	report_start = get_lock_dependencies_report_start(start, size, data);
	if(report_start == -1) {
		return;
	}

	sprintf(filename, "/data/ers_lock_dependencies_%d", atomic_read(&report_num));

	oldfs = get_fs();
	set_fs(get_ds());

	fd = sys_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0766);
	if (fd < 0) {
		return;
	}
	atomic_inc(&report_num);

	if (report_start < start) {
		sys_write(fd, &data[report_start], start - report_start);
	} else {
		sys_write(fd, &data[report_start], size - report_start);
		sys_write(fd, &data[0], start);
	}
	
	sys_close(fd);
	
	set_fs(oldfs);
}

EXPORT_SYMBOL(lock_dependencies_report);


int get_panic_report_start(uint32_t start, uint32_t size, uint8_t *data)
{
	int report_start;
	int i;	

	report_start = -1;

	for (i = start - 1; i > -1; --i) {
		if (data[i] == 'C') {
			if (!strncmp(&data[i], "CPU:", 4)) {
				report_start = i;
				break;
			}
		}
	}

	if (i > -1) {
		return report_start;		
	}

	for (i = size - 1; i >= start; --i) {
		if (data[i] == 'C') {
			if (!strncmp(&data[i], "CPU:", 4)) {
				report_start = i;
				break;
			}
		}
	}

	if (i < start) {
		return -1;
	}

	return report_start;
}

#if 0
void panic_report(void)
{
	int fd, fd_dump;
	int value;
	mm_segment_t oldfs;

	uint32_t start;
	uint32_t size;
	uint8_t *data;
	int report_start;

	value = atomic_read(&enable);
	if (value == 0) {
		return;
	}

	ram_console_buffer = get_ram_console_buffer();
	if (!ram_console_buffer) {
		return;
	}
	
	start = ram_console_buffer->start; 
	size = ram_console_buffer->size;
	data = ram_console_buffer->data;
	
	report_start = get_panic_report_start(start, size, data);
	if(report_start == -1) {
		return;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	fd = sys_open("/data/ers_panic", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	fd_dump = sys_open("/data/ers_panic_dump", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
		return;
	}

	if (report_start < start) {
		sys_write(fd, &data[report_start], start - report_start);
		sys_write(fd_dump, &data[report_start], start - report_start);
	} else {
		sys_write(fd, &data[report_start], size - report_start);
		sys_write(fd_dump, &data[report_start], size - report_start);
		sys_write(fd, &data[0], start);
		sys_write(fd_dump, &data[0], start);
	}
	
	sys_close(fd);
	
	set_fs(oldfs);
}
#endif

void panic_report(void)
{
	int fd, fd_dump;
	int value;
	mm_segment_t oldfs;

	uint32_t start;
	uint32_t size;
	uint8_t *data;
	int report_start;

	value = atomic_read(&enable);
	if (value == 0) {
		return;
	}

	ram_console_buffer = get_ram_console_buffer();
	if (!ram_console_buffer) {
		return;
	}
	
	start = ram_console_buffer->start; 
	size = ram_console_buffer->size;
	data = ram_console_buffer->data;
	
	report_start = get_panic_report_start(start, size, data);
	if(report_start == -1) {
		return;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	fd = sys_open("/data/ers_panic", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	fd_dump = sys_open("/data/ers_panic_dump", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
		return;
	}

	sys_write(fd, &data[0], size);
	sys_write(fd_dump, &data[0], size);
	
	sys_close(fd);
	set_fs(oldfs);
}
EXPORT_SYMBOL(panic_report);

static ssize_t ers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value = atomic_read(&enable);
	if (value == 0) {
		printk("The ers of kernel was disabled.\n");
	} else {
		printk("The ers of kernel was enabled.\n");
	}
	return value;
}

static ssize_t ers_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	atomic_set(&enable, value);

	return size;
}

static DEVICE_ATTR(ers, S_IRUGO | S_IWUGO, ers_show, ers_store);

static ssize_t ers_panic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int *value = 0x1000;
	*value = 100;
	return size;
}

static DEVICE_ATTR(ers_panic, S_IRUGO | S_IWUGO, 0, ers_panic_store);

static int __devinit ers_probe(struct platform_device *pdev)
{
	int ret;
	ret = device_create_file(&pdev->dev, &dev_attr_ers);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}
	ret = device_create_file(&pdev->dev, &dev_attr_ers_panic);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}

	return ret;
}

static int __devexit ers_remove(struct platform_device *pdev)
{	
	device_remove_file(&pdev->dev, &dev_attr_ers);
	device_remove_file(&pdev->dev, &dev_attr_ers_panic);

	return 0;
}

static struct platform_driver ers_driver = {
	.probe = ers_probe,
	.remove = __devexit_p(ers_remove),
	.driver = {
		.name = ERS_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ers_init(void)
{
	return platform_driver_register(&ers_driver);
}
module_init(ers_init);

static void __exit ers_exit(void)
{
	platform_driver_unregister(&ers_driver);
}
module_exit(ers_exit);

MODULE_DESCRIPTION("Griffin Exception Reporting System Driver");
MODULE_AUTHOR("Jun-Yeong Han <j.y.han@lge.com>");
MODULE_LICENSE("GPL");
