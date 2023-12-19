# ESP32 Zephyr blinky

## Intro
Simple blinky using Zephyr RTOS stack on ESP32 platform.

## Procedure

```
west build -b esp32_devkitc_wroom -p
west build -b esp32_devkitc_wroom
west espressif monitor
```

## Quick overview

### Task 1
Custom overlay file that modifies 2 ESP32 pins for input (button) and output (LED). &#9745;

ReactClass to handle simple example using *k_work_delayable* Zephyr structure that blinks LED. &#9745;

PollClass that reads button state and sends it to the Zbus software bus. &#9745;

ReactClass listener that handles ZBus events. &#9745;

ReactClass *k_work_delayable* timer delay update. &#9746;

I attached the demonstration video.

### Task 2
I haven't managed to successfully finish the first task and since I don't have 
experience with google tests framework and time left to research I decided to 
skip second task.