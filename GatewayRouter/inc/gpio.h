/**
  * @file           : gpio.h
  * @brief          : Function definitions for communicating with RPi GPIO pins (to toggle LEDs)
  * @author         : Arden Diakhate-Palme 
  * @date           : 14.12.22
  * 
**/
#ifndef _GPIO_H_
#define _GPIO_H_

void init_GPIO();
void close_GPIO();

void set_green_led();
void clear_green_led();
void set_red_led();
void clear_red_led();
int get_button_val();


#endif