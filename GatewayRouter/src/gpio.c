/**
  * @file           : gpio.c
  * @brief          : Functions for communicating with RPi GPIO pins (to toggle LEDs)
  *                   Using the PIGPIO library: https://abyz.me.uk/rpi/pigpio/
  * @author         : Arden Diakhate-Palme 
  * @date           : 14.12.22
  * 
**/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pigpio.h>
#include <string.h>

#include "../inc/gpio.h"

const int GREEN_LED = 21;
const int RED_LED = 26;
const int BUTTON = 12;

void init_GPIO() {
    if (gpioInitialise() < 0){
       fprintf(stderr,"GPIO failed to initalize\n");
    }

    gpioSetMode(GREEN_LED, PI_OUTPUT);
    gpioSetMode(RED_LED, PI_OUTPUT);
    gpioSetPullUpDown(GREEN_LED, PI_PUD_OFF); 
    gpioSetPullUpDown(RED_LED, PI_PUD_OFF); 
}

void close_GPIO() {
    gpioTerminate();
}


void set_green_led() {
    gpioWrite(GREEN_LED, 1);
}

void clear_green_led() {
    gpioWrite(GREEN_LED, 0);
}

void set_red_led() {
    gpioWrite(RED_LED, 1);
}

void clear_red_led() {
    gpioWrite(RED_LED, 0);
}

int get_button_val() {
    int res;
    res = gpioRead(BUTTON);
    return res;
}