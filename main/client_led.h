#ifndef CLIENT_LED_H
#define CLIENT_LED_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define LED_STRIP_GPIO GPIO_NUM_8 // GPIO pin for LED strip

void led_init();
void set_led_color(int red, int green, int blue);
void turn_off_led();



#endif