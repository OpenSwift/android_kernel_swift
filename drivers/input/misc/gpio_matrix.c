/* drivers/input/misc/gpio_matrix.c
 *
 * Copyright (C) 2007 Google, Inc.
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
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
#include <mach/gpio.h>
#endif /* CONFIG_MACH_MSM7X27_SWIFT */
#include <linux/at_kpd_eve.h>

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
#include <linux/delay.h>
#endif /* CONFIG_MACH_MSM7X27_SWIFT */
struct gpio_kp {
	struct gpio_event_input_devs *input_devs;
	struct gpio_event_matrix_info *keypad_info;
	struct hrtimer timer;
	struct wake_lock wake_lock;
	int current_output;
	unsigned int use_irq:1;
	unsigned int key_state_changed:1;
	unsigned int last_key_state_changed:1;
	unsigned int some_keys_pressed:2;
	unsigned long keys_pressed[0];
};
#if defined(CONFIG_MACH_MSM7X27_SWIFT)
struct input_dev *at_input_dev;

static const unsigned short at_kbd_keycode[] = {
		KEY_PREVIOUSSONG,	KEY_RECORD		,	KEY_STOPCD	,	KEY_PHONE		,	KEY_CLOSECD
	,	KEY_EJECTCD		,	KEY_EJECTCLOSECD,	KEY_REWIND	,	KEY_NEXTSONG	,	KEY_PLAYPAUSE
	,	KEY_ISO			,	KEY_CONFIG		,	KEY_HOMEPAGE,	KEY_REFRESH		,	KEY_EXIT
	,	KEY_MOVE		,	KEY_1			,	KEY_2		,	KEY_3			,	KEY_4
	,	KEY_5			,	KEY_6			,	KEY_7		,	KEY_8			,	KEY_9
	,	KEY_0			,	227	/* '*' */	,	228 /* '#' */,	KEY_A			,	KEY_B
	,	KEY_C			,	KEY_D			,	KEY_E		,	KEY_F			,	KEY_G
	,	KEY_H			,	KEY_I			,	KEY_J		,	KEY_K			,	KEY_L
	,	KEY_M			,	KEY_N			,	KEY_O		,	KEY_P			,	KEY_Q
	,	KEY_R			,	KEY_S			,	KEY_T		,	KEY_U			,	KEY_V
	,	KEY_W			,	KEY_X			,	KEY_Y		,	KEY_Z			,	KEY_MAX  
};

#endif /* CONFIG_MACH_MSM7X27_SWIFT */

extern int keypad_event;

static void clear_phantom_key(struct gpio_kp *kp, int out, int in)
{
	struct gpio_event_matrix_info *mi = kp->keypad_info;
	int key_index = out * mi->ninputs + in;
	unsigned short keyentry = mi->keymap[key_index];
	unsigned short keycode = keyentry & MATRIX_KEY_MASK;
	unsigned short dev = keyentry >> MATRIX_CODE_BITS;

	if (!test_bit(keycode, kp->input_devs->dev[dev]->key)) {
		if (mi->flags & GPIOKPF_PRINT_PHANTOM_KEYS)
			pr_info("gpiomatrix: phantom key %x, %d-%d (%d-%d) "
				"cleared\n", keycode, out, in,
				mi->output_gpios[out], mi->input_gpios[in]);
		__clear_bit(key_index, kp->keys_pressed);
	} else {
		if (mi->flags & GPIOKPF_PRINT_PHANTOM_KEYS)
			pr_info("gpiomatrix: phantom key %x, %d-%d (%d-%d) "
				"not cleared\n", keycode, out, in,
				mi->output_gpios[out], mi->input_gpios[in]);
	}
}

static int restore_keys_for_input(struct gpio_kp *kp, int out, int in)
{
	int rv = 0;
	int key_index;

	key_index = out * kp->keypad_info->ninputs + in;
	while (out < kp->keypad_info->noutputs) {
		if (test_bit(key_index, kp->keys_pressed)) {
			rv = 1;
			clear_phantom_key(kp, out, in);
		}
		key_index += kp->keypad_info->ninputs;
		out++;
	}
	return rv;
}

static void remove_phantom_keys(struct gpio_kp *kp)
{
	int out, in, inp;
	int key_index;

	if (kp->some_keys_pressed < 3)
		return;

	for (out = 0; out < kp->keypad_info->noutputs; out++) {
		inp = -1;
		key_index = out * kp->keypad_info->ninputs;
		for (in = 0; in < kp->keypad_info->ninputs; in++, key_index++) {
			if (test_bit(key_index, kp->keys_pressed)) {
				if (inp == -1) {
					inp = in;
					continue;
				}
				if (inp >= 0) {
					if (!restore_keys_for_input(kp, out + 1,
									inp))
						break;
					clear_phantom_key(kp, out, inp);
					inp = -2;
				}
				restore_keys_for_input(kp, out, in);
			}
		}
	}
}

static void report_key(struct gpio_kp *kp, int key_index, int out, int in)
{
	struct gpio_event_matrix_info *mi = kp->keypad_info;
	int pressed = test_bit(key_index, kp->keys_pressed);
	unsigned short keyentry = mi->keymap[key_index];
	unsigned short keycode = keyentry & MATRIX_KEY_MASK;
	unsigned short dev = keyentry >> MATRIX_CODE_BITS;

	if (pressed != test_bit(keycode, kp->input_devs->dev[dev]->key)) {
		if (keycode == KEY_RESERVED) {
			if (mi->flags & GPIOKPF_PRINT_UNMAPPED_KEYS)
				pr_info("gpiomatrix: unmapped key, %d-%d "
					"(%d-%d) changed to %d\n",
					out, in, mi->output_gpios[out],
					mi->input_gpios[in], pressed);
		} else {
			if (mi->flags & GPIOKPF_PRINT_MAPPED_KEYS)
				pr_info("gpiomatrix: key %x, %d-%d (%d-%d) "
					"changed to %d\n", keycode,
					out, in, mi->output_gpios[out],
					mi->input_gpios[in], pressed);

			if (pressed == 1) {
				switch (keycode) {
				case 115:	/* VOLUME UP */ 
					write_gkpd_value(85);
					break;	
				case 114:   /* VOLUME DOWN */ 
					write_gkpd_value(68);
					break;	
				case 231:   /* SEND */ 
					write_gkpd_value(83);
					break;	
				case 139:   /* MENU */ 
					write_gkpd_value(77);
					break;	
				case 217:   /* SEARCH */ 
					write_gkpd_value(65);
					break;	
				case 242:   /* FOCUS */ 
					write_gkpd_value(70);
					break;	
				case 212:   /* CAMERA */ 
					write_gkpd_value(67);
					break;	
				}
			}

			input_report_key(kp->input_devs->dev[dev], keycode, pressed);
		}
	}
}

static enum hrtimer_restart gpio_keypad_timer_func(struct hrtimer *timer)
{
	int out, in;
	int key_index;
	int gpio;
	int gpio34, gpio35, gpio36, gpio37, gpio38;
	struct gpio_kp *kp = container_of(timer, struct gpio_kp, timer);
	struct gpio_event_matrix_info *mi = kp->keypad_info;
	unsigned gpio_keypad_flags = mi->flags;
	unsigned polarity = !!(gpio_keypad_flags & GPIOKPF_ACTIVE_HIGH);

	out = kp->current_output;
	if (out == mi->noutputs) {
		out = 0;
		kp->last_key_state_changed = kp->key_state_changed;
		kp->key_state_changed = 0;
		kp->some_keys_pressed = 0;
	} else {
		key_index = out * mi->ninputs;
		for (in = 0; in < mi->ninputs; in++, key_index++) {
			gpio = mi->input_gpios[in];
		
			if (key_index == 3 && test_bit(key_index, kp->keys_pressed)) {
				gpio34 = gpio_get_value(34);
				gpio35 = gpio_get_value(35);
				gpio36 = gpio_get_value(36);
				
				gpio_set_value(34, 0);
				gpio_set_value(35, 0);
				gpio_set_value(36, 0);
				gpio37 = gpio_get_value(37);
				if (gpio37 == 0) {
					gpio_set_value(34, 1);
					gpio_set_value(35, 0);
					gpio_set_value(36, 1);
					gpio38 = gpio_get_value(38);
					if (gpio38 == 0) {
						/* printk(KERN_INFO "Capture key pressed detected\n"); */
					} else {
						hrtimer_start(timer, mi->settle_time, HRTIMER_MODE_REL);
						return HRTIMER_NORESTART;
					}
				} 
				gpio_set_value(34, gpio34);
				gpio_set_value(35, gpio35);
				gpio_set_value(36, gpio36);
			}

			if (gpio_get_value(gpio) ^ !polarity) {
				if (kp->some_keys_pressed < 3)
					kp->some_keys_pressed++;
				kp->key_state_changed |= !__test_and_set_bit(
						key_index, kp->keys_pressed);
			} else
				kp->key_state_changed |= __test_and_clear_bit(
						key_index, kp->keys_pressed);
		}
		gpio = mi->output_gpios[out];
		if (gpio_keypad_flags & GPIOKPF_DRIVE_INACTIVE)
			gpio_set_value(gpio, !polarity);
		else
			gpio_direction_input(gpio);
		out++;
	}
	kp->current_output = out;
	if (out < mi->noutputs) {
		gpio = mi->output_gpios[out];
		if (gpio_keypad_flags & GPIOKPF_DRIVE_INACTIVE)
			gpio_set_value(gpio, polarity);
		else
			gpio_direction_output(gpio, polarity);
		
		hrtimer_start(timer, mi->settle_time, HRTIMER_MODE_REL);
		return HRTIMER_NORESTART;
	}
	if (gpio_keypad_flags & GPIOKPF_DEBOUNCE) {
		if (kp->key_state_changed) {
			hrtimer_start(&kp->timer, mi->debounce_delay,
				      HRTIMER_MODE_REL);
			return HRTIMER_NORESTART;
		}
		kp->key_state_changed = kp->last_key_state_changed;
	}
	if (kp->key_state_changed) {
		if (gpio_keypad_flags & GPIOKPF_REMOVE_SOME_PHANTOM_KEYS)
			remove_phantom_keys(kp);
		key_index = 0;
		for (out = 0; out < mi->noutputs; out++)
			for (in = 0; in < mi->ninputs; in++, key_index++)
				report_key(kp, key_index, out, in);
	}
	if (!kp->use_irq || kp->some_keys_pressed) {
		hrtimer_start(timer, mi->poll_time, HRTIMER_MODE_REL);
		return HRTIMER_NORESTART;
	}

	keypad_event = 0;

	/* No keys are pressed, reenable interrupt */
	for (out = 0; out < mi->noutputs; out++) {
		if (gpio_keypad_flags & GPIOKPF_DRIVE_INACTIVE)
			gpio_set_value(mi->output_gpios[out], polarity);
		else
			gpio_direction_output(mi->output_gpios[out], polarity);
	}

	for (in = 0; in < mi->ninputs; in++)
		enable_irq(gpio_to_irq(mi->input_gpios[in]));
	wake_unlock(&kp->wake_lock);

	return HRTIMER_NORESTART;
}

static irqreturn_t gpio_keypad_irq_handler(int irq_in, void *dev_id)
{
	int i;
	struct gpio_kp *kp = dev_id;
	struct gpio_event_matrix_info *mi = kp->keypad_info;
	unsigned gpio_keypad_flags = mi->flags;

	keypad_event = 1;

	if (!kp->use_irq) /* ignore interrupt while registering the handler */
		return IRQ_HANDLED;
	
	printk(KERN_INFO"%s: \n",__func__);
	for (i = 0; i < mi->ninputs; i++) {
		disable_irq(gpio_to_irq(mi->input_gpios[i]));
	}
	for (i = 0; i < mi->noutputs; i++) {
		if (gpio_keypad_flags & GPIOKPF_DRIVE_INACTIVE)
			gpio_set_value(mi->output_gpios[i],
				!(gpio_keypad_flags & GPIOKPF_ACTIVE_HIGH));
		else
			gpio_direction_input(mi->output_gpios[i]);
	}
	wake_lock(&kp->wake_lock);

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
	hrtimer_start(&kp->timer, ktime_set(0, 100 * 1000000), HRTIMER_MODE_REL);
#else /* qualcomm or google */
	hrtimer_start(&kp->timer, ktime_set(0, 10000), HRTIMER_MODE_REL);
#endif /* CONFIG_MACH_MSM7X27_SWIFT */
	return IRQ_HANDLED;
}

static int gpio_keypad_request_irqs(struct gpio_kp *kp)
{
	int i;
	int err;
	unsigned int irq;
	unsigned long request_flags;
	struct gpio_event_matrix_info *mi = kp->keypad_info;

	switch (mi->flags & (GPIOKPF_ACTIVE_HIGH|GPIOKPF_LEVEL_TRIGGERED_IRQ)) {
	default:
		request_flags = IRQF_TRIGGER_FALLING;
		break;
	case GPIOKPF_ACTIVE_HIGH:
		request_flags = IRQF_TRIGGER_RISING;
		break;
	case GPIOKPF_LEVEL_TRIGGERED_IRQ:
		request_flags = IRQF_TRIGGER_LOW;
		break;
	case GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_ACTIVE_HIGH:
		request_flags = IRQF_TRIGGER_HIGH;
		break;
	}

	for (i = 0; i < mi->ninputs; i++) {
#if defined(CONFIG_MACH_MSM7X27_SWIFT)
		gpio_tlmm_config(GPIO_CFG(mi->input_gpios[i], 0, GPIO_INPUT, GPIO_PULL_UP,
						                GPIO_2MA), GPIO_ENABLE);
#endif /* CONFIG_MACH_MSM7X27_SWIFT */
		err = irq = gpio_to_irq(mi->input_gpios[i]);
		if (err < 0)
			goto err_gpio_get_irq_num_failed;
		err = request_irq(irq, gpio_keypad_irq_handler, request_flags,
				  "gpio_kp", kp);
		if (err) {
			pr_err("gpiomatrix: request_irq failed for input %d, "
				"irq %d\n", mi->input_gpios[i], irq);
			goto err_request_irq_failed;
		}
		
		if (mi->input_gpios[i] == 37) {
			err = set_irq_wake(irq, 1);
			if (err) {
				pr_err("gpiomatrix: set_irq_wake failed for input %d, "
					"irq %d\n", mi->input_gpios[i], irq);
			}
		} else {
			err = set_irq_wake(irq, 1);
			if (err) {
				pr_err("gpiomatrix: set_irq_wake failed for input %d, "
					"irq %d\n", mi->input_gpios[i], irq);
			}
		}
		
		disable_irq(irq);
	}

	return 0;

	for (i = mi->noutputs - 1; i >= 0; i--) {
		free_irq(gpio_to_irq(mi->input_gpios[i]), kp);
err_request_irq_failed:
err_gpio_get_irq_num_failed:
		;
	}
	
	return err;
}

int gpio_event_matrix_func(struct gpio_event_input_devs *input_devs,
	struct gpio_event_info *info, void **data, int func)
{
	int i;
	int err;
	int key_count;
	struct gpio_kp *kp;
	struct gpio_event_matrix_info *mi;

	mi = container_of(info, struct gpio_event_matrix_info, info);

#if 0
//#if define (CONFIG_MACH_MSM7X27_SWIFT)_
	kp = *data;
	if (func == GPIO_EVENT_FUNC_SUSPEND) {
		if (kp->use_irq)
			for (i = 0; i < mi->ninputs; i++)
				disable_irq(gpio_to_irq(mi->input_gpios[i]));

		hrtimer_cancel(&kp->timer);
		return 0;
	}
	if (func == GPIO_EVENT_FUNC_RESUME) {
		hrtimer_start(&kp->timer, ktime_set(0, 10000), HRTIMER_MODE_REL);
		return 0;
	}
#else /* qualcomm or google */
	if (func == GPIO_EVENT_FUNC_SUSPEND || func == GPIO_EVENT_FUNC_RESUME) {
		/* TODO: disable scanning */
		return 0;
	}
#endif /* CONFIG_MACH_MSM7X27_SWIFT */

	if (func == GPIO_EVENT_FUNC_INIT) {
		if (mi->keymap == NULL ||
		   mi->input_gpios == NULL ||
		   mi->output_gpios == NULL) {
			err = -ENODEV;
			pr_err("gpiomatrix: Incomplete pdata\n");
			goto err_invalid_platform_data;
		}
		key_count = mi->ninputs * mi->noutputs;

		*data = kp = kzalloc(sizeof(*kp) + sizeof(kp->keys_pressed[0]) *
				     BITS_TO_LONGS(key_count), GFP_KERNEL);
		if (kp == NULL) {
			err = -ENOMEM;
			pr_err("gpiomatrix: Failed to allocate private data\n");
			goto err_kp_alloc_failed;
		}
		kp->input_devs = input_devs;
		kp->keypad_info = mi;
		for (i = 0; i < key_count; i++) {
			unsigned short keyentry = mi->keymap[i];
			unsigned short keycode = keyentry & MATRIX_KEY_MASK;
			unsigned short dev = keyentry >> MATRIX_CODE_BITS;

			if (dev >= input_devs->count) {
				pr_err("gpiomatrix: bad device index %d >= "
					"%d for key code %d\n",
					dev, input_devs->count, keycode);
				err = -EINVAL;
				goto err_bad_keymap;
			}
			at_input_dev = input_devs->dev[dev];	
			if (keycode && keycode <= KEY_MAX)	
				input_set_capability(input_devs->dev[dev],
							EV_KEY, keycode);					

		}
#if defined(CONFIG_MACH_MSM7X27_SWIFT)

	for(i=0; i<(sizeof(at_kbd_keycode)/sizeof(unsigned short)), at_kbd_keycode[i] != KEY_MAX; i++)
	{
		input_set_capability(at_input_dev, EV_KEY, at_kbd_keycode[i] & KEY_MAX);
	}
        
#endif /* CONFIG_MACH_MSM7X27_SWIFT */
		for (i = 0; i < mi->noutputs; i++) {
			if (gpio_cansleep(mi->output_gpios[i])) {
				pr_err("gpiomatrix: unsupported output gpio %d,"
					" can sleep\n", mi->output_gpios[i]);
				err = -EINVAL;
				goto err_request_output_gpio_failed;
			}
			err = gpio_request(mi->output_gpios[i], "gpio_kp_out");
			if (err) {
				pr_err("gpiomatrix: gpio_request failed for "
					"output %d\n", mi->output_gpios[i]);
				goto err_request_output_gpio_failed;
			}
			if (mi->flags & GPIOKPF_DRIVE_INACTIVE)
				err = gpio_direction_output(mi->output_gpios[i],
					!(mi->flags & GPIOKPF_ACTIVE_HIGH));
			else
				err = gpio_direction_input(mi->output_gpios[i]);
			if (err) {
				pr_err("gpiomatrix: gpio_configure failed for "
					"output %d\n", mi->output_gpios[i]);
				goto err_output_gpio_configure_failed;
			}
		}
		for (i = 0; i < mi->ninputs; i++) {
			err = gpio_request(mi->input_gpios[i], "gpio_kp_in");
			if (err) {
				pr_err("gpiomatrix: gpio_request failed for "
					"input %d\n", mi->input_gpios[i]);
				goto err_request_input_gpio_failed;
			}
			err = gpio_direction_input(mi->input_gpios[i]);
			if (err) {
				pr_err("gpiomatrix: gpio_direction_input failed"
					" for input %d\n", mi->input_gpios[i]);
				goto err_gpio_direction_input_failed;
			}
		}
		kp->current_output = mi->noutputs;
		kp->key_state_changed = 1;

		hrtimer_init(&kp->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		kp->timer.function = gpio_keypad_timer_func;
		wake_lock_init(&kp->wake_lock, WAKE_LOCK_SUSPEND, "gpio_kp");
		err = gpio_keypad_request_irqs(kp);
		kp->use_irq = err == 0;

		pr_info("GPIO Matrix Keypad Driver: Start keypad matrix for "
			"%s%s in %s mode\n", input_devs->dev[0]->name,
			(input_devs->count > 1) ? "..." : "",
			kp->use_irq ? "interrupt" : "polling");

		if (kp->use_irq)
			wake_lock(&kp->wake_lock);
		hrtimer_start(&kp->timer, ktime_set(0, 10000),
				 HRTIMER_MODE_REL);

		return 0;
	}

	err = 0;
	kp = *data;

	if (kp->use_irq)
		for (i = mi->noutputs - 1; i >= 0; i--)
			free_irq(gpio_to_irq(mi->input_gpios[i]), kp);

	hrtimer_cancel(&kp->timer);
	wake_lock_destroy(&kp->wake_lock);
	for (i = mi->noutputs - 1; i >= 0; i--) {
err_gpio_direction_input_failed:
		gpio_free(mi->input_gpios[i]);
err_request_input_gpio_failed:
		;
	}
	for (i = mi->noutputs - 1; i >= 0; i--) {
err_output_gpio_configure_failed:
		gpio_free(mi->output_gpios[i]);
err_request_output_gpio_failed:
		;
	}
err_bad_keymap:
	kfree(kp);
err_kp_alloc_failed:
err_invalid_platform_data:
	return err;
}

#if defined(CONFIG_MACH_MSM7X27_SWIFT)
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
#define KEY_INPUT_DELAY     10
//int eve_flash_set_led_state(int led_state);
void lge_atcmd_report_key(unsigned int mode, unsigned int arg)
{	
        printk("lge_atcmd_report_key: Mode %u, argument: %u \n", mode, arg);

        switch(mode)
        {
		
        	case ATCMD_LCD_REPORTKEY:
                switch(arg) {
                    case 0: // Init
                        input_report_key(at_input_dev, KEY_ISO, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_ISO, 0);
                        break;
                    case 2: // Tilt
                        input_report_key(at_input_dev, KEY_CONFIG, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_CONFIG, 0);
                        break;
                    case 3: // Color
                        input_report_key(at_input_dev, KEY_HOMEPAGE, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_HOMEPAGE, 0);
                        break;
                    case 11: // Red
                        input_report_key(at_input_dev, KEY_REFRESH, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_REFRESH, 0);
                        break;
                    case 12: // Green
                        input_report_key(at_input_dev, KEY_EXIT, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_EXIT, 0);
                        break;
                    case 13: // Blue
                        input_report_key(at_input_dev, KEY_MOVE, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_MOVE, 0);
                        break;
                    default:
                        printk("lge_atcmd_report_key: invalid parameters\n");
                }
                return;
                break;

        }
        switch (arg) {
                case CAM_AVR_INITIAL:
                        input_report_key(at_input_dev, KEY_PREVIOUSSONG, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_PREVIOUSSONG, 0);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_PREVIOUSSONG, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_PREVIOUSSONG, 0);
                        break;
                case CAM_AVR_ON:
                        if (mode) {
                                /* Camcorder */
                                input_report_key(at_input_dev, KEY_RECORD, 1);
                                mdelay(KEY_INPUT_DELAY);
                                input_report_key(at_input_dev, KEY_RECORD, 0);
                        } else {
                                /* Camera */
                                input_report_key(at_input_dev, KEY_STOPCD, 1);
                                mdelay(KEY_INPUT_DELAY);
                                input_report_key(at_input_dev, KEY_STOPCD, 0);
                        }
                        break;
                case CAM_AVR_SHOT:
                        if (mode) {
                                /* Camcorder */
                                input_report_key(at_input_dev, KEY_PHONE, 1);
                                mdelay(KEY_INPUT_DELAY);
                                input_report_key(at_input_dev, KEY_PHONE, 0);
                        } else {
                                /* Camera */
                                input_report_key(at_input_dev, KEY_PHONE, 1);
                                mdelay(KEY_INPUT_DELAY);
                                input_report_key(at_input_dev, KEY_PHONE, 0);
                        }
                        break;
                case CAM_AVR_SAVE:
                        if (mode) {
                                /* Camcorder */
                                input_report_key(at_input_dev, KEY_PHONE, 1);
                                mdelay(KEY_INPUT_DELAY);
                                input_report_key(at_input_dev, KEY_PHONE, 0);
                        }
                        break;
                case CAM_AVR_CALL:
                        input_report_key(at_input_dev, KEY_CLOSECD, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_CLOSECD, 0);
                        break;
                case CAM_AVR_ERASE:
                        input_report_key(at_input_dev, KEY_EJECTCD, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_EJECTCD, 0);
                        break;
                case AVR_FLASH_ON:
                    #ifdef CONFIG_MSM_CAMERA
//                        eve_flash_set_led_state(2);
                    #endif
                        break;
                case AVR_FLASH_OFF:
                    #ifdef CONFIG_MSM_CAMERA
//                        eve_flash_set_led_state(3);
                    #endif
                        break;
                case CAM_STROBE_ON:
                        input_report_key(at_input_dev, KEY_EJECTCLOSECD, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_EJECTCLOSECD, 0);
                        break;
                case CAM_STROBE_OFF:
                        input_report_key(at_input_dev, KEY_REWIND, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_REWIND, 0);
                        break;
                case CAM_ZOOM_ON:
                        input_report_key(at_input_dev, KEY_NEXTSONG, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_NEXTSONG, 0);
                        break;
                case CAM_ZOOM_OFF:
                        input_report_key(at_input_dev, KEY_PLAYPAUSE, 1);
                        mdelay(KEY_INPUT_DELAY);
                        input_report_key(at_input_dev, KEY_PLAYPAUSE, 0);
                        break;
                default:
                        printk("lge_atcmd_report_key: invalid parameters\n");
        }
}
EXPORT_SYMBOL(lge_atcmd_report_key);
#endif /* CONFIG_MACH_EVE */

