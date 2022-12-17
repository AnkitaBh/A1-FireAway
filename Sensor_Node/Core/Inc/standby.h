/*
 * standby.h
 *
 *  Created on: Nov 12, 2022
 *      Author: karenabruzzo
 */

#ifndef INC_STANDBY_H_
#define INC_STANDBY_H_

void enter_standby(RTC_HandleTypeDef hrtc, UART_HandleTypeDef huart2,  uint16_t myTimer);

#endif /* INC_STANDBY_H_ */
