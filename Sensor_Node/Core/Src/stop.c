/*
 * stop.c
 *
 *  Created on: Nov 21, 2022
 *      Author: karenabruzzo
 *
 *  *  Modified by: Arden Diakhate-Palme
 *           on: 07.12.22
 */

#include "main.h"
#include "string.h"
#include "prints.h"

extern int PRINT_TIME;

extern RTC_HandleTypeDef hrtc;

extern UART_HandleTypeDef huart2;


void enter_stop(uint16_t myTimer){

	 /* Display the string */
	HAL_printf("About to enter the STOP MODE\n\n");
    //uint16_t myTimer =  0x3000;
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, myTimer,  RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0) != HAL_OK)
    {
      Error_Handler();
    }

	 /****** Suspend the Ticks before entering the STOP mode or else this can wake the device up **********/
	 HAL_SuspendTick();

	 /* Enter Stop Mode */
	 HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	 //exit stop mode
	 HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
	 HAL_ResumeTick();
     HAL_printf("WAKEUP FROM RTC\n\n");
}
