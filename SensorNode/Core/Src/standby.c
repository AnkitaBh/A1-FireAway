/*
 * standby.c
 *
 *  Created on: Nov 12, 2022
 *      Author: karenabruzzo
 * 
 *  Modified by: Arden Diakhate-Palme
 *           on: 07.12.22
 */
#include "main.h"
#include "prints.h"

extern int PRINT_TIME;


//RTC_HandleTypeDef hrtc;


//UART_HandleTypeDef huart2;

void enter_standby(RTC_HandleTypeDef hrtc, UART_HandleTypeDef huart2,  uint16_t myTimer) {

	     /* Clear the WU FLAG */
	    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	     /* clear the RTC Wake UP (WU) flag */
	    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);


	     /* Display the string */
	    HAL_printf("About to enter the STANDBY MODE\n\n");


	     /* Enable the WAKEUP PIN */
	    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

	    /* enable the RTC Wakeup */
	    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, myTimer,  RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0) != HAL_OK)
	        {
	          Error_Handler();
	        }

	     /* one last string to be sure */
	    HAL_printf("STANDBY MODE is ON\n\n");

	     /* Finally enter the standby mode */
	    HAL_PWR_EnterSTANDBYMode();

}
