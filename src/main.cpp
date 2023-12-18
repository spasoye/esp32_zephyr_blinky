/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define POLL_STACK_SIZE 500
#define POLL_THREAD_PRIORITY 5
#define SLEEP_TIME_MS   1000

K_THREAD_STACK_DEFINE(poll_thread_stack, POLL_STACK_SIZE);

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE 	DT_ALIAS(led0)
/* The devicetree node identifier for the "button0" alias. */
#define BTN0_NODE	DT_ALIAS(button0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);

class PollClass{
public:
    PollClass(const struct gpio_dt_spec *btn)
    {
        printf("Initializing poll class\n");
        
        btn_dev = btn;

        poll_thread_id = k_thread_create(&poll_thread_data,
                                         poll_thread_stack,
                                         K_THREAD_STACK_SIZEOF(poll_thread_stack),
                                         PollClass::poll_thread_handler,
                                         this, NULL, NULL, POLL_THREAD_PRIORITY,
                                         0, K_NO_WAIT);

        k_thread_join(&poll_thread_data,K_FOREVER);
    }
private:
    // Poll thread variables
    struct k_thread poll_thread_data;
    k_tid_t poll_thread_id;

    const struct gpio_dt_spec *btn_dev;

    bool btn_prev_state;

   /**
    * @brief  Static bridge function to non static member main function.
    * @note   Still dont understand this
    * @param  *object: 
    * @param  *: 
    * @param  *: 
    * @retval None
    */
    static void poll_thread_handler(void *object, void *, void *)
    {
        auto threadObject = reinterpret_cast<PollClass*>(object);
	    threadObject->main();
    }

    void main()
    {
        bool btn_curr_state = gpio_pin_get_dt(btn_dev);
        btn_prev_state = gpio_pin_get_dt(btn_dev);

        while (1) {
            // ret = gpio_pin_toggle_dt(&led);
            // if (ret < 0) {
            //     return 0;
            // }
            
            btn_curr_state = gpio_pin_get_dt(btn_dev);
            
            printf("Button state: %s\n", btn_curr_state ? "ON" : "OFF");
            // 

            btn_prev_state = btn_curr_state;
            k_msleep(SLEEP_TIME_MS);
        }
    }
};

class ReactClass{
public:
    /**
     * @brief  
     * @note   
     * @param  *dt_led: pointer to GPIO device tree node
     * @retval 
     */
    ReactClass(const struct gpio_dt_spec *dt_led)
    {
        this->led_state = false;
        this->timeout_ms = 100;
        this->led_dev = dt_led;

        k_work_init_delayable(&led_timer, &ReactClass::blink_handler_bridge);
        k_work_reschedule(&led_timer, K_MSEC(timeout_ms));
    }

private:
    struct k_work_delayable led_timer;	
    const struct gpio_dt_spec *led_dev;

    uint16_t timeout_ms;
    bool led_state;

    // Static function that acts as a bridge to the non-static member function
    static void blink_handler_bridge(struct k_work *work) {
        // TODO: WTF research this 
        ReactClass *reactObj = reinterpret_cast<ReactClass*>(work);
        if (reactObj) {
            reactObj->blink_handler(work);
        }
    }
    

    void blink_handler(struct k_work *work)
    {
        int ret;
        led_state = !led_state;
        

        ret = gpio_pin_toggle_dt(led_dev);
        printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_work_reschedule(&led_timer, K_MSEC(timeout_ms));
    }
};


int main(void)
{
    int ret;

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
    PollClass check_gumbek(&btn);
    while (true)
    {
        // printf ("Main func\n");
        k_msleep(SLEEP_TIME_MS);
    }
    
    return 0;
}
