/* arch/arm/mach-msm/lge_battery.c
 *
 * Copyright (C) 2008 Google, Inc.
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
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/power_supply.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <mach/board.h>

static struct wake_lock vbus_wake_lock;

/* TODO: temporary work struct for battery information updating instead of using rpc */
static struct delayed_work batt_work;

#define TRACE_BATT 0

#if TRACE_BATT
#define BATT(x...) printk(KERN_INFO "[BATT] " x)
#else
#define BATT(x...) do {} while (0)
#endif

/* module debugger */
#define LGE_BATTERY_DEBUG		1
#define BATTERY_PREVENTION		1

/* Enable this will shut down if no battery */
#define ENABLE_BATTERY_DETECTION	0

typedef enum {
	DISABLE = 0,
	ENABLE_SLOW_CHG,
	ENABLE_FAST_CHG
} batt_ctl_t;

/* This order is the same as lge_power_supplies[]
 * And it's also the same as lge_cable_status_update()
 */
typedef enum {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC
} charger_type_t;

struct battery_info_reply {
	u32 batt_id;		/* Battery ID from ADC */
	u32 batt_vol;		/* Battery voltage from ADC */
	u32 batt_temp;		/* Battery Temperature (C) from formula and ADC */
	u32 batt_current;	/* Battery current from ADC */
	u32 level;		/* formula */
	u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
	u32 charging_enabled;	/* 0: Disable, 1: Enable */
	u32 full_bat;		/* Full capacity of battery (mAh) */
};

struct lge_battery_info {
	int present;
	unsigned long update_time;

	/* lock to protect the battery info */
	struct mutex lock;

	struct battery_info_reply rep;
};

static struct lge_battery_info lge_batt_info;

static unsigned int cache_time = 1000;

static int lge_battery_initial = 0;

static enum power_supply_property lge_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property lge_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

/* HTC dedicated attributes */
static ssize_t lge_battery_show_property(struct device *dev,
					  struct device_attribute *attr,
					  char *buf);

static int lge_power_get_property(struct power_supply *psy, 
				    enum power_supply_property psp,
				    union power_supply_propval *val);

static int lge_battery_get_property(struct power_supply *psy, 
				    enum power_supply_property psp,
				    union power_supply_propval *val);

static struct power_supply lge_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = lge_battery_properties,
		.num_properties = ARRAY_SIZE(lge_battery_properties),
		.get_property = lge_battery_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = lge_power_properties,
		.num_properties = ARRAY_SIZE(lge_power_properties),
		.get_property = lge_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = lge_power_properties,
		.num_properties = ARRAY_SIZE(lge_power_properties),
		.get_property = lge_power_get_property,
	},
};


/* -------------------------------------------------------------------------- */

#if defined(CONFIG_DEBUG_FS)
int lge_battery_set_charging(batt_ctl_t ctl);
static int batt_debug_set(void *data, u64 val)
{
	return lge_battery_set_charging((batt_ctl_t) val);
}

static int batt_debug_get(void *data, u64 *val)
{
	return -ENOSYS;
}

DEFINE_SIMPLE_ATTRIBUTE(batt_debug_fops, batt_debug_get, batt_debug_set, "%llu\n");
static int __init batt_debug_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("lge_battery", 0);
	if (IS_ERR(dent))
		return PTR_ERR(dent);

	debugfs_create_file("charger_state", 0644, dent, NULL, &batt_debug_fops);

	return 0;
}

device_initcall(batt_debug_init);
#endif

/* 
 *	battery_charging_ctrl - battery charing control.
 * 	@ctl:			battery control command
 *
 */
static int battery_charging_ctrl(batt_ctl_t ctl)
{
	int result = 0;

	switch (ctl) {
	case DISABLE:
		BATT("charger OFF\n");
		/* 0 for enable; 1 disable */
		break;
	case ENABLE_SLOW_CHG:
		BATT("charger ON (SLOW)\n");
		break;
	case ENABLE_FAST_CHG:
		BATT("charger ON (FAST)\n");
		break;
	default:
		printk(KERN_ERR "Not supported battery ctr called.!\n");
		result = -EINVAL;
		break;
	}
	
	return result;
}

int lge_battery_set_charging(batt_ctl_t ctl)
{
	int rc;
	
	if ((rc = battery_charging_ctrl(ctl)) < 0)
		goto result;
	
	if (!lge_battery_initial) {
		lge_batt_info.rep.charging_enabled = ctl & 0x3;
	} else {
		mutex_lock(&lge_batt_info.lock);
		lge_batt_info.rep.charging_enabled = ctl & 0x3;
		mutex_unlock(&lge_batt_info.lock);
	}
result:	
	return rc;
}

int lge_battery_status_update(u32 curr_level)
{
	if (!lge_battery_initial)
		return 0;

	power_supply_changed(&lge_power_supplies[CHARGER_BATTERY]);
	
	return 0;
}

int lge_cable_status_update(int status)
{
	int rc = 0;
	unsigned source;

	if (!lge_battery_initial)
		return 0;
	
	mutex_lock(&lge_batt_info.lock);
	switch(status) {
	case CHARGER_BATTERY:
		BATT("cable NOT PRESENT\n");
		lge_batt_info.rep.charging_source = CHARGER_BATTERY;
		break;
	case CHARGER_USB:
		BATT("cable USB\n");
		lge_batt_info.rep.charging_source = CHARGER_USB;
		break;
	case CHARGER_AC:
		BATT("cable AC\n");
		lge_batt_info.rep.charging_source = CHARGER_AC;
		break;
	default:
		printk(KERN_ERR "%s: Not supported cable status received!\n",
				__FUNCTION__);
		rc = -EINVAL;
	}
	source = lge_batt_info.rep.charging_source;
	mutex_unlock(&lge_batt_info.lock);

	msm_hsusb_set_vbus_state(source == CHARGER_USB);
	if (source == CHARGER_USB) {
		wake_lock(&vbus_wake_lock);
	} else {
		/* give userspace some time to see the uevent and update
		 * LED state or whatnot...
		 */
		wake_lock_timeout(&vbus_wake_lock, HZ / 2);
	}

	/* if the power source changes, all power supplies may change state */
	power_supply_changed(&lge_power_supplies[CHARGER_BATTERY]);
	power_supply_changed(&lge_power_supplies[CHARGER_USB]);
	power_supply_changed(&lge_power_supplies[CHARGER_AC]); 
	return rc;
}

static int lge_get_batt_info(struct battery_info_reply *buffer)
{
/* TODO: temporary work struct for battery information updating instead of using rpc */
	mutex_lock(&lge_batt_info.lock);
	buffer->level 				= 50;
	mutex_unlock(&lge_batt_info.lock);
	
	return 0;
}

static int lge_power_get_property(struct power_supply *psy, 
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	charger_type_t charger;
	
	mutex_lock(&lge_batt_info.lock);
	charger = lge_batt_info.rep.charging_source;
	mutex_unlock(&lge_batt_info.lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS)
			val->intval = (charger ==  CHARGER_AC ? 1 : 0);
		else if (psy->type == POWER_SUPPLY_TYPE_USB)
			val->intval = (charger ==  CHARGER_USB ? 1 : 0);
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

static int lge_battery_get_charging_status(void)
{
	u32 level;
	charger_type_t charger;	
	int ret;
	
	mutex_lock(&lge_batt_info.lock);
	charger = lge_batt_info.rep.charging_source;
	
	switch (charger) {
	case CHARGER_BATTERY:
		ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case CHARGER_USB:
	case CHARGER_AC:
		level = lge_batt_info.rep.level;
		if (level == 100)
			ret = POWER_SUPPLY_STATUS_FULL;
		else
			ret = POWER_SUPPLY_STATUS_CHARGING;
		break;
	default:
		ret = POWER_SUPPLY_STATUS_UNKNOWN;
	}
	mutex_unlock(&lge_batt_info.lock);
	return ret;
}

static int lge_battery_get_property(struct power_supply *psy, 
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = lge_batt_info.present;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		mutex_lock(&lge_batt_info.lock);
		val->intval = lge_batt_info.rep.level;
		mutex_unlock(&lge_batt_info.lock);
		break;
	default:		
		return -EINVAL;
	}
	
	return 0;
}

#define LGE_BATTERY_ATTR(_name)							\
{										\
	.attr = { .name = #_name, .mode = S_IRUGO, .owner = THIS_MODULE },	\
	.show = lge_battery_show_property,					\
	.store = NULL,								\
}

static struct device_attribute lge_battery_attrs[] = {
	LGE_BATTERY_ATTR(batt_id),
	LGE_BATTERY_ATTR(batt_vol),
	LGE_BATTERY_ATTR(batt_temp),
	LGE_BATTERY_ATTR(batt_current),
	LGE_BATTERY_ATTR(charging_source),
	LGE_BATTERY_ATTR(charging_enabled),
	LGE_BATTERY_ATTR(full_bat),
};

enum {
	BATT_ID = 0,
	BATT_VOL,
	BATT_TEMP,
	BATT_CURRENT,
	CHARGING_SOURCE,
	CHARGING_ENABLED,
	FULL_BAT,
};

static ssize_t lge_battery_set_delta(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long delta = 0;
	
	delta = simple_strtoul(buf, NULL, 10);

	if (delta > 100)
		return -EINVAL;

	return count;
}

static struct device_attribute lge_set_delta_attrs[] = {
	__ATTR(delta, S_IWUSR | S_IWGRP, NULL, lge_battery_set_delta),
};

static int lge_battery_create_attrs(struct device * dev)
{
	int i, j, rc;
	
	for (i = 0; i < ARRAY_SIZE(lge_battery_attrs); i++) {
		rc = device_create_file(dev, &lge_battery_attrs[i]);
		if (rc)
			goto lge_attrs_failed;
	}

	for (j = 0; j < ARRAY_SIZE(lge_set_delta_attrs); j++) {
		rc = device_create_file(dev, &lge_set_delta_attrs[j]);
		if (rc)
			goto lge_delta_attrs_failed;
	}
	
	goto succeed;
	
lge_attrs_failed:
	while (i--)
		device_remove_file(dev, &lge_battery_attrs[i]);
lge_delta_attrs_failed:
	while (j--)
		device_remove_file(dev, &lge_set_delta_attrs[i]);
succeed:	
	return rc;
}

static ssize_t lge_battery_show_property(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - lge_battery_attrs;
	
	/* check cache time to decide if we need to update */
	if (lge_batt_info.update_time &&
            time_before(jiffies, lge_batt_info.update_time +
                                msecs_to_jiffies(cache_time)))
                goto dont_need_update;
	
	if (lge_get_batt_info(&lge_batt_info.rep) < 0) {
		printk(KERN_ERR "%s: rpc failed!!!\n", __FUNCTION__);
	} else {
		lge_batt_info.update_time = jiffies;
	}
dont_need_update:

	mutex_lock(&lge_batt_info.lock);
	switch (off) {
	case BATT_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       lge_batt_info.rep.batt_id);
		break;
	case BATT_VOL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       lge_batt_info.rep.batt_vol);
		break;
	case BATT_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       lge_batt_info.rep.batt_temp);
		break;
	case BATT_CURRENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       lge_batt_info.rep.batt_current);
		break;
	case CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       lge_batt_info.rep.charging_source);
		break;
	case CHARGING_ENABLED:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       lge_batt_info.rep.charging_enabled);
		break;		
	case FULL_BAT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       lge_batt_info.rep.full_bat);
		break;
	default:
		i = -EINVAL;
	}	
	mutex_unlock(&lge_batt_info.lock);
	
	return i;
}

static int lge_battery_probe(struct platform_device *pdev)
{
	int i, rc;

	/* init structure data member */
	lge_batt_info.update_time 	= jiffies;
	lge_batt_info.present 		= 1;
	
	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++) {
		rc = power_supply_register(&pdev->dev, &lge_power_supplies[i]);
		if (rc)
			printk(KERN_ERR "Failed to register power supply (%d)\n", rc);	
	}

	/* create lge detail attributes */
	lge_battery_create_attrs(lge_power_supplies[CHARGER_BATTERY].dev);

	lge_battery_initial = 1;

	if (lge_get_batt_info(&lge_batt_info.rep) < 0)
		printk(KERN_ERR "%s: get info failed\n", __FUNCTION__);

	lge_cable_status_update(lge_batt_info.rep.charging_source);
	battery_charging_ctrl(lge_batt_info.rep.charging_enabled ?
			      ENABLE_SLOW_CHG : DISABLE);

	lge_batt_info.update_time = jiffies;

	if (lge_batt_info.rep.charging_enabled == 0)
		battery_charging_ctrl(DISABLE);
	
	return 0;
}

static struct platform_driver lge_battery_driver = {
	.probe	= lge_battery_probe,
	.driver	= {
		.name	= "lge-battery",
		.owner	= THIS_MODULE,
	},
};

/* TODO: temporary work struct for battery information updating instead of using rpc */
static void batt_work_func(struct work_struct *work)
{
	lge_battery_status_update(50);

	schedule_delayed_work(&batt_work, HZ/2);

	return;
}

static int __init lge_battery_init(void)
{
	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
	mutex_init(&lge_batt_info.lock);

	/* TODO: temporary work struct for battery information updating instead of using rpc */
	INIT_DELAYED_WORK(&batt_work, batt_work_func);
	schedule_delayed_work(&batt_work, HZ/2);
	
	platform_driver_register(&lge_battery_driver);
	return 0;
}

module_init(lge_battery_init);
MODULE_DESCRIPTION("LGE Battery Driver");
MODULE_LICENSE("GPL");

