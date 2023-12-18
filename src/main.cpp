/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/zbus/zbus.h>

#define POLL_STACK_SIZE 500
#define POLL_THREAD_PRIORITY 5
K_THREAD_STACK_DEFINE(poll_thread_stack, POLL_STACK_SIZE);

#define REACT_STACK_SIZE 500
#define REACT_THREAD_PRIORITY 4
K_THREAD_STACK_DEFINE(react_thread_stack, REACT_STACK_SIZE);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE 	DT_ALIAS(led0)
/* The devicetree node identifier for the "button0" alias. */
#define BTN0_NODE	DT_ALIAS(button0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);

// zbus pin status
struct pin_status
{
    bool value;
    bool changed;
};

class PollClass{
public:
    PollClass(const struct gpio_dt_spec *btn, const struct zbus_channel *zbus_chan_arg)
    {
        printf("Initializing poll class\n");

        zbus_chan = zbus_chan_arg;
        btn_dev = btn;

        poll_thread_id = k_thread_create(&poll_thread_data,
                                         poll_thread_stack,
                                         K_THREAD_STACK_SIZEOF(poll_thread_stack),
                                         PollClass::poll_thread_handler,
                                         this, NULL, NULL, POLL_THREAD_PRIORITY,
                                         0, K_NO_WAIT);
    }

    void start(void)
    {
        k_thread_join(&poll_thread_data,K_FOREVER);
    }
private:
    struct pin_status zbus_btn_state;
    
    const struct zbus_channel *zbus_chan;
    // Poll thread variables
    struct k_thread poll_thread_data;
    k_tid_t poll_thread_id;

    const struct gpio_dt_spec *btn_dev;

    bool btn_prev_state;

   /**
    * @brief  Static bridge function to non static member main function.
    * @note   Still dont understand this
    * @param  *object: 
    * * @param  *: 
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
            btn_curr_state = gpio_pin_get_dt(btn_dev);
            
            zbus_btn_state.value = btn_curr_state;

            // check if value is changed
            if (btn_curr_state != btn_prev_state)
            {
                printf("Button state: %s\n", btn_curr_state ? "ON" : "OFF");
                zbus_btn_state.changed = true;
            }
            else
            {
                zbus_btn_state.changed = false;
            }
        
            int pub_ret = zbus_chan_pub(zbus_chan, &zbus_btn_state, K_MSEC(SLEEP_TIME_MS));

            if (pub_ret != 0)
            {
                printf("Error publishing\n");
            }

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

    void start(void)
    {
        k_thread_join(&react_thread_data,K_FOREVER);
    }

    /**
     * @brief  
     * @note   
     * @param  *chan: 
     * @retval None
     */
    void zbus_listener_callback(const struct zbus_channel *chan)
    {
        const struct pin_status *pin = static_cast<const struct pin_status*>(zbus_chan_const_msg(chan));

        printf("From listener\n");
        
        if (pin->changed){
            timeout_ms = timeout_ms + 50;
        }
    }

private:
    struct k_work_delayable led_timer;	
    const struct gpio_dt_spec *led_dev;

    // React thread variables
    // IF DEFINED BEFORE struct k_work_delayable led_timer PROGRAM FAILS ??
    struct k_thread react_thread_data;
    k_tid_t react_thread_id;
    
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
        //printf("LED state: %s\n", led_state ? "ON" : "OFF");
        k_work_reschedule(&led_timer, K_MSEC(timeout_ms));
    }

    /**
    * @brief  Static bridge function to non static member main function.
    * @note   Still dont understand this
    * @param  *object: 
    * @param  *: 
    * @param  *: 
    * @retval None
    */
    static void react_thread_handler(void *object, void *, void *)
    {
        auto threadObject = reinterpret_cast<ReactClass*>(object);
	    threadObject->main();
    }

    void main()
    {
        // subscribe to zbus
        while (true)
        {
            printf("waiting for zbus packets\n");
            k_msleep(SLEEP_TIME_MS);
        }    
    }
};

static void listener_callback_bridge(const struct zbus_channel *chan) {
        ReactClass *react_obj = static_cast<ReactClass*>(zbus_chan_user_data(chan));
        if (react_obj) {
            react_obj->zbus_listener_callback(chan);
        }
    }

ZBUS_CHAN_DEFINE(pin_status_chan,  /* Name */
		 struct pin_status, /* Message type */

		 NULL,                                 /* Validator */
		 NULL,                                 /* User data */
		 ZBUS_OBSERVERS(pin_lis),     /* observers */
		 ZBUS_MSG_INIT(.value = false, .changed = true) /* Initial value */
);

ZBUS_LISTENER_DEFINE(pin_lis, listener_callback_bridge);

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

    ret = zbus_chan_add_obs(&pin_status_chan, &pin_lis, K_MSEC(200));
    if (ret != 0)
    {
        printf("Add an observer to channel error");
    }

    PollClass check_gumbek(&btn, &pin_status_chan);
    ReactClass ledica(&led);
    

    check_gumbek.start();


    while (true)
    {
        // printf ("Main func\n");
        k_msleep(SLEEP_TIME_MS);
    }
    
    return 0;
}
