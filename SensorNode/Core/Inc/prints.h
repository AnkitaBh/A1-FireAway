/**
  * @file           : prints.c
  * @brief          : Definitions for print statements functions using STM32's UART 
  *                   Note that this code was adapted from https://notes.iopush.net/blog/2016/stm32-log-and-printf/
  * @author         : Arden Diakhate-Palme, Oliv' 
  * @date           : 14.12.22
  * 
**/

#ifndef __PRINTS_H__
#define __PRINTS_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "stm32l4xx_hal.h"

/** Custom printf function, only translate to va_list arg HAL_UART.
 * @param *fmt String to print
 * @param ... Data
 */
void HAL_printf(const char *fmt, ...);

void wait_ms(uint32_t timeout);

#endif