/**
  * @file           : prints.c
  * @brief          : Print statements functions using STM32's UART 
  *                   Note that this code was adapted from https://notes.iopush.net/blog/2016/stm32-log-and-printf/
  * @author         : Arden Diakhate-Palme, Oliv' 
  * @date           : 14.12.22
  * 
**/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "stm32l4xx_hal.h"
#include "prints.h"

#define PRINT_BUFFER_SIZE 240

extern UART_HandleTypeDef huart2;
extern uint32_t tim2_tick_count;


/** Custom printf function, only translate to va_list arg HAL_UART.
 * @param *fmt String to print
 * @param ... Data
 */
void HAL_printf(const char *fmt, ...) {
  char string[PRINT_BUFFER_SIZE];
  va_list argp;
  va_start(argp, fmt);
  vsprintf(string, fmt, argp);
  HAL_UART_Transmit_IT(&huart2, (uint8_t*)string, strlen(string));
  va_end(argp);
  wait_ms(10);
}

void wait_ms(uint32_t timeout) {
    uint32_t start_time = tim2_tick_count;
    while((tim2_tick_count - start_time) < timeout);
}
