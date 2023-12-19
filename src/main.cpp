/**
 * @brief  
 * @note   
 * @retval 
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/zbus/zbus.h>

#define POLL_STACK_SIZE 500
#define POLL_THREAD_PRIORITY 5
K_THREAD_STACK_DEFINE(poll_thread_stack, POLL_STACK_SIZE);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

// Device tree stuff
/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE 	DT_ALIAS(led0)
/* The devicetree node identifier for the "button0" alias. */
#define BTN0_NODE	DT_ALIAS(button0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);

// ZBUS stuff
static void listener_callback_bridge(const struct zbus_channel *chan);
struct pin_status
{
    bool value;
    bool changed;
};

ZBUS_CHAN_DEFINE(pin_status_chan,  /* Name */
		 struct pin_status, /* Message type */

		 NULL,                                 /* Validator */
		 NULL,                                 /* User data */
		 ZBUS_OBSERVERS(pin_lis),     /* observers */
		 ZBUS_MSG_INIT(.value = false, .changed = true) /* Initial value */
);

ZBUS_LISTENER_DEFINE(pin_lis, listener_callback_bridge);

/**
 * @brief lass for polling button state and publishing it to a Zephyr Zbus 
 * channel.
 * Initializes a Zephyr thread to continuously poll the state of a button
 * and publish the state to a Zbus channel.
 */
class PollClass{
public:
    /**
     * @brief Construct a new Poll Class object. 
     * @param btn Pointer to the GPIO device tree specification for the button. 
     * @param zbus_chan_arg Pointer to Zbus channel.
     */
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

    /**
     * @brief Starts the PollClass thread.
     * 
     */
    void start(void)
    {
        k_thread_join(&poll_thread_data,K_FOREVER);
    }
private:
    struct pin_status zbus_btn_state;       /* */
    
    const struct zbus_channel *zbus_chan;   /* Pointer to Zbus channel. */
    
    // Poll thread variables
    struct k_thread poll_thread_data;       /* Poll thread data structure. */
    k_tid_t poll_thread_id;                 /* Thread ID for poll thread. */

    const struct gpio_dt_spec *btn_dev;     /* Pointer to GPIO device tree  
                                                button specification*/

    bool btn_prev_state;                    /* Previous button state */

   /**
    * @brief  Static bridge function to non static member main function.
    * @note 
    * @param  *object Pointer to the PollClass object
    * @param  * Not used
    * @param  * Not used 
    * @retval None
    */
    static void poll_thread_handler(void *object, void *, void *)
    {
        auto threadObject = reinterpret_cast<PollClass*>(object);
	    threadObject->main();
    }

    /**
     * @brief Main function in the poll thread.
     * 
     * Polls the status of button GPIO every 1 second, check if state has 
     * changed and publishes 
     */
    void main()
    {
        bool btn_curr_state = gpio_pin_get_dt(btn_dev);
        btn_prev_state = gpio_pin_get_dt(btn_dev);

        while (1) {           
            btn_curr_state = gpio_pin_get_dt(btn_dev);
            
            zbus_btn_state.value = btn_curr_state;

            // check if value is changed.
            if (btn_curr_state != btn_prev_state)
            {
                printf("Button state: %s\n", btn_curr_state ? "ON" : "OFF");
                zbus_btn_state.changed = true;
            }
            else
            {
                zbus_btn_state.changed = false;
            }
        
            int pub_ret = zbus_chan_pub(zbus_chan, &zbus_btn_state, K_FOREVER);

            if (pub_ret != 0)
            {
                printf("Error publishing\n");
            }

            btn_prev_state = btn_curr_state;
            k_msleep(100);
        }
    }
};

/**
 * @brief Class in charge of LED control and ZBus event handling. 
 * 
 */
class ReactClass{
public:
    /**
     * @brief  Constructor for the ReactClass.
     * @note   Initializes led_state, timer delay in ms and DT GPIO node.
     *         It also setups delay timer and starts it.
     * @param  *dt_led: pointer to GPIO device tree node for LED.
     * @retval None
     */
    ReactClass(const struct gpio_dt_spec *dt_led)
    {
        printf("Initializing ReactClass\n");
        this->led_state = false;
        this->delay_ms = 100;
        this->led_dev = dt_led;

        k_work_init_delayable(&led_timer, &ReactClass::blink_handler_bridge);
        k_work_reschedule(&led_timer, K_MSEC(delay_ms));

    }

    /**
     * @brief  Callback function for handling ZBus events.
     * @note   Still WIP
     * @param  *chan: 
     * @retval None
     */
    void zbus_listener_callback(const struct zbus_channel *chan)
    {
        const struct pin_status *pin = reinterpret_cast<const struct pin_status*>(zbus_chan_const_msg(chan));

        // if (pin) {
        //     printf("-> Button state: %s\n -> Changed state: %s /n", pin->value ? "ON" : "OFF", pin->changed ? "YES" : "NO");
        // } else {
        //     printf("Failed to cast zbus_chan_const_msg to pin_status*\n");
        // }
        // If button changed k_work delay should be increased 
        // TODO H
        if (pin->changed && pin->value){
            printf("Promjena\n");
            // increase_delay()
        }
    }

private:
    struct k_work_delayable led_timer;	
    const struct gpio_dt_spec *led_dev;

    // React thread variables
    // IF DEFINED BEFORE struct k_work_delayable led_timer PROGRAM FAILS ??
    struct k_thread react_thread_data;
    k_tid_t react_thread_id;
    
    bool led_state;
    uint16_t delay_ms;

    // void increase_delay()
    // {
    //     printf("increase_delay: delay_ms:\n" );
    // }

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
        printf("blink_handler: delay_ms: %d\n", delay_ms);
        k_work_reschedule(&led_timer, K_MSEC(delay_ms));
    }
};

static void listener_callback_bridge(const struct zbus_channel *chan) {
        ReactClass *react_obj = static_cast<ReactClass*>(zbus_chan_user_data(chan));

        react_obj->zbus_listener_callback(chan);
}

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

    PollClass check_gumbek(&btn, &pin_status_chan);
    ReactClass ledica(&led);

    check_gumbek.start();

    printf("Tu sam1\n");

    ret = zbus_chan_add_obs(&pin_status_chan, &pin_lis, K_FOREVER);
    if (ret != 0)
    {
        printf("Add an observer to channel error %d \n", ret);
    }

    while (true)
    {
        printf ("Main func\n");
        k_msleep(SLEEP_TIME_MS);
    }
    
    return 0;
}
