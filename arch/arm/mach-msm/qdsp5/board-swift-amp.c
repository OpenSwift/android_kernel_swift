/* arch/arm/mach-msm/board-swift.c
 *
 * Copyright (C) 2009 LGE, Inc.
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/gpio.h>
#include <asm/system.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#define MODULE_NAME	"amp_wm9093"

#define	DEBUG_AMP_CTL	1

static uint32_t msm_snd_debug = 1;
module_param_named(debug_mask, msm_snd_debug, uint, 0664);


#if DEBUG_AMP_CTL
#define D(fmt, args...) printk(fmt, ##args)
#else
#define D(fmt, args...) do {} while(0)
#endif

struct amp_data {
	struct i2c_client *client;
};

static struct amp_data *_data = NULL;


int amp_read_register(u8 reg, int* ret)
{
	//ret = swab16(i2c_smbus_read_word_data(_data->client, reg));
	struct i2c_msg	xfer[2];
	u16				data = 0xffff;
	u16				retval;


	xfer[0].addr = _data->client->addr;
	xfer[1].flags = 0;
	xfer[0].len  = 1;
	xfer[0].buf = &reg;

	xfer[1].addr = _data->client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = 2;
	xfer[1].buf = (u8*)&data;

	retval = i2c_transfer(_data->client->adapter, xfer, 2);

	*ret =  (data>>8) | ((data & 0xff) << 8);

	return retval;
}

int amp_write_register(u8 reg, int value)
{
	//if (i2c_smbus_write_word_data(_data->client, reg, swab16(value)) < 0)
    //			printk(KERN_INFO "========== woonrae: i2c error==================");
	
	int				 err;
	unsigned char    buf[3];
	struct i2c_msg	msg = { _data->client->addr, 0, 3, &buf[0] }; 
	
	buf[0] = reg;
	buf[1] = (value & 0xFF00) >> 8;
	buf[2] = value & 0x00FF;

	if ((err = i2c_transfer(_data->client->adapter, &msg, 1)) < 0){
			if (msm_snd_debug & 1)
				printk(KERN_INFO "AMP i2c write error! \n");
	}
	else{
			if (msm_snd_debug & 1)
				printk(KERN_INFO "AMP i2c write ok\n");
	}

	return 0;
}



static void swift_init_client(struct i2c_client *client)
{
	struct amp_data *data = i2c_get_clientdata(client);

	amp_write_register(0x00, 0x9093);
	printk(KERN_INFO "AMP INIT OK\n");
}

static int swift_amp_ctl_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct amp_data *data;
	struct i2c_adapter* adapter = client->adapter;
	int err;
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA)){
		err = -EOPNOTSUPP;
		return err;
	}

	if (msm_snd_debug & 1)
		printk(KERN_INFO "%s()\n", __FUNCTION__);
	
	data = kzalloc(sizeof (struct amp_data), GFP_KERNEL);
	if (NULL == data) {
			return -ENOMEM;
	}
	_data = data;
	data->client = client;
	i2c_set_clientdata(client, data);
	
	if (msm_snd_debug & 1)
		printk(KERN_INFO "%s chip found\n", client->name);
	
	swift_init_client(client);
	
	return 0;
}

static int swift_amp_ctl_remove(struct i2c_client *client)
{
	struct amp_data *data = i2c_get_clientdata(client);
	kfree (data);
	
	printk(KERN_INFO "%s()\n", __FUNCTION__);
	i2c_set_clientdata(client, NULL);
	return 0;
}


static struct i2c_device_id swift_amp_idtable[] = {
	{ "amp_wm9093", 0 },
};

static struct i2c_driver swift_amp_ctl_driver = {
	.probe = swift_amp_ctl_probe,
	.remove = swift_amp_ctl_remove,
	.id_table = swift_amp_idtable,
	.driver = {
		.name = MODULE_NAME,
	},
};


static int __init swift_amp_ctl_init(void)
{
	return i2c_add_driver(&swift_amp_ctl_driver);	
}

static void __exit swift_amp_ctl_exit(void)
{
	return i2c_del_driver(&swift_amp_ctl_driver);
}

module_init(swift_amp_ctl_init);
module_exit(swift_amp_ctl_exit);

MODULE_DESCRIPTION("SWIFT Amp Control");
MODULE_AUTHOR("Woonrae Cho <woonrae@lge.com>");
MODULE_LICENSE("GPL");
