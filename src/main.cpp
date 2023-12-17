/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE 	DT_ALIAS(led0)

/* The devicetree node identifier for the "button0" alias. */
#define BTN0_NODE	DT_ALIAS(button0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);

class PollClass{
    // TODO
};


class ReactClass{
public:
    ReactClass(const struct gpio_dt_spec *dt_led)
    {
        this->led_state = false;
        this->timeout_ms = 12345;
        this->led_dev = dt_led;

        k_work_init_delayable(&led_timer, blink_handler);
    }

private:
    struct k_work_delayable led_timer;	
    const struct gpio_dt_spec *led_dev;

    uint16_t timeout_ms;
    bool led_state;

    // Static function that acts as a bridge to the non-static member function
    static void blink_handler_bridge(struct k_work *work) {
        ReactClass *reactObj = reinterpret_cast<ReactClass*>(work->node);
        reactObj->blink_handler(work);
    }

    void blink_handler(struct k_work *work)
    {
        ReactClass *reactObj = reinterpret_cast<ReactClass*>(work);

        int ret;
        led_state = !led_state;
        

        ret = gpio_pin_toggle_dt(led_dev);

        printf("LED state: %s\n", led_state ? "ON" : "OFF");

        k_work_reschedule(&led_timer, K_MSEC(500));
    }
};


int main(void)
{
    int ret;
    bool led_state = true;
    bool btn_state = false;

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    if (ret < 0) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&btn, GPIO_INPUT);

    if (ret < 0) {
        return 0;
    }

    ReactClass ledica(&led);


    while (1) {
        // ret = gpio_pin_toggle_dt(&led);
        // if (ret < 0) {
        //     return 0;
        // }
        
        btn_state = gpio_pin_get_dt(&btn);
        
        // led_state = !led_state;
        // printf("LED state: %s\n", led_state ? "ON" : "OFF");
        printf("Button state: %s\n", btn_state ? "ON" : "OFF");

        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
