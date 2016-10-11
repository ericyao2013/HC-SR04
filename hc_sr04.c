/**
 * @file Sample app to utilize GPIO on and Arduino Due.
 *
 *
 */

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <gpio.h>
#include <sys_clock.h>
#include <misc/util.h>
#include <limits.h>

#if defined(CONFIG_SOC_ATMEL_SAM3)
#define GPIO_OUT_PIN		16
#define GPIO_INT_PIN		17
#define GPIO_OUT_LED_PIN	18
#define GPIO_NAME		"GPIO_"
#endif

#if defined(CONFIG_GPIO_DW_0)
#define GPIO_DRV_NAME CONFIG_GPIO_DW_0_NAME
#elif defined(CONFIG_GPIO_QMSI_0)
#define GPIO_DRV_NAME CONFIG_GPIO_QMSI_0_NAME
#elif defined(CONFIG_GPIO_ATMEL_SAM3)
#define GPIO_DRV_NAME CONFIG_GPIO_ATMEL_SAM3_PORTC_DEV_NAME
#else
#define GPIO_DRV_NAME "GPIO_0"
#endif

#define MAX_RANGE	20

/* Macro */
#define HW_CYCLES_TO_USEC(__hw_cycle__) \
        ( \
                ((uint64_t)(__hw_cycle__) * (uint64_t)sys_clock_us_per_tick) / \
                ((uint64_t)sys_clock_hw_cycles_per_tick) \
        )


/* Inline functions */
static inline uint32_t time_delta(uint32_t ts, uint32_t t)
{
        return (t >= ts) ? (t - ts) : (ULONG_MAX - ts + t);
}


void main(void)
{
	struct device *gpio_dev;
	int ret;
	int val;


	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		printk("Cannot find %s!\n", GPIO_DRV_NAME);
	}

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_dev, GPIO_OUT_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
	}

	/* Setup GPIO input, and triggers on rising edge. */
	ret = gpio_pin_configure(gpio_dev, GPIO_INT_PIN,
			(GPIO_DIR_IN | GPIO_INT_EDGE
			 | GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_INT_PIN);
	}

	/* Setup GPIO LED output */
	ret = gpio_pin_configure(gpio_dev, GPIO_OUT_LED_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
	}


	while (1) {

 		/* The following trigPin/echoPin cycle is used to determine the
 		 * distance of the nearest object by bouncing soundwaves off of it. 
		 */		
		ret = gpio_pin_write(gpio_dev, GPIO_OUT_PIN, 0);
		if (ret) {
			printk("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
		}
		task_sleep(USEC(30));

		ret = gpio_pin_write(gpio_dev, GPIO_OUT_PIN, 1);
		if (ret) {
			printk("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
		}
		task_sleep(USEC(10));

		ret = gpio_pin_write(gpio_dev, GPIO_OUT_PIN, 0);
		if (ret) {
			printk("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
		}
		
		printk("Reading \n");
		do {
			gpio_pin_read(gpio_dev, GPIO_INT_PIN, &val); 
		} while (val == 0);
		uint32_t start = sys_cycle_get_32();

		do {
			gpio_pin_read(gpio_dev, GPIO_INT_PIN, &val); 
		} while (val == 1);
		uint32_t end = sys_cycle_get_32();
		//uint32_t duration = end - start;
		uint32_t duration;
		duration = HW_CYCLES_TO_USEC(time_delta(start, end));
		printk("duration %d \n", duration);
		//duration = (uint32_t)duration /
		//	   (uint32_t)sys_clock_hw_cycles_per_tick; 

                //Calculate the distance (in cm) based on the speed of sound.
		uint32_t distance = duration / 58;
		printk("Distance %d \n", distance);

		if (distance >= MAX_RANGE) {
			printk("Out Of Range\n");
			ret = gpio_pin_write(gpio_dev, GPIO_OUT_LED_PIN, 0);
			if (ret) {
				printk("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
			}
		} else {
			printk("In range\n");
			ret = gpio_pin_write(gpio_dev, GPIO_OUT_LED_PIN, 1);
			if (ret) {
				printk("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
			}
		}
		task_sleep(MSEC(100));
	}
}
