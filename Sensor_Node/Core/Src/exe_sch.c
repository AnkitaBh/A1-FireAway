/*
 * exe_sch.c
 *
 *  Created on: Nov 16, 2022
 *      Author: karenabruzzo
 * 
 *  Modified 23.11.22 by Arden Diakhate-Palme 
 */

/*typedef struct {
    // same number of TS as there are nodes in the network  - 1 (to exclude GW) (b/c each node TX's in a given TS)
    uint16_t total_ts;          //total number of absolute timeslots;
    //(will have to add this remaining sleep time to the total sleep time after phase 4)
    timeslot_t tx_ts;           // each node only transmits data once back up to its parent
    uint16_t num_rx_ts;
    timeslot_t *rx_ts;          // [0, 1]
    uint16_t num_ts_sleep;      // number of timeslots to sleep for after transmitting
} data_sch_config_t ;*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "protocols.h"
#include "main.h"
#include "lora.h"
#include "uart.h"
#include "exe_sch.h"
#include "standby.h"
#include "prints.h"

extern int PRINT_TIME;

extern TIM_HandleTypeDef htim16;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
extern ADC_HandleTypeDef hadc1;
extern uint64_t tim2_tick_count;
extern RTC_HandleTypeDef hrtc;

extern bool data_phase;
extern int DATA_TS;
extern int DC_TS; 
extern int LORA_TOA; 
extern int TIME_BUF; 
extern int TEMP_THRESH;
extern int DATA_RECV_WIN;

void detect_fire(mixnet_node_config_t *config, data_config_t *sensor_db, uint64_t timespan);

void exe_sch() {
    char printf_buf[40];
    HAL_TIM_Base_Stop_IT(&htim16);
    __HAL_TIM_SET_AUTORELOAD(&htim16, DATA_TS - 1);
    __HAL_TIM_SET_COUNTER(&htim16, 0);
    data_phase = true;
    HAL_printf("Executing the schedule\n");
    // wait_ms(PRINT_TIME);

    HAL_TIM_Base_Start_IT(&htim16);
}

void check_data_schedule(mixnet_node_config_t *config, mixnet_address parent_addr, data_sch_config_t sch_config, data_config_t *sensor_db) {
    char printf_buf[40];
    bool idle_ts = true;
    static int curr_data_ts = -1;
    //uint64_t start_time = 0;
    
    char *raw_packet;
    int recv_port = 0;
    mixnet_packet_t *recvd_packet;
    packet_data_t *recvd_data_pkt;

    // check if we've executed the entire schedule already
    if(curr_data_ts == sch_config.total_ts) {
        idle_ts = false;
        HAL_TIM_Base_Stop_IT(&htim16);
        data_phase = false;
        return;
    }

    if(curr_data_ts != -1){
        HAL_printf("timeslot: %d\n", curr_data_ts);
    }

    if (curr_data_ts == sch_config.tx_ts) {
        idle_ts = false;
        detect_fire(config, sensor_db, DC_TS);
        print_sensor_db(sensor_db);

        HAL_printf("transmitting data to %u\n", parent_addr);
        // wait_ms(PRINT_TIME);
        send_sensor_data(config, sensor_db, parent_addr);

    } else if(curr_data_ts > sch_config.tx_ts){
        idle_ts = true;
        // int16_t standby_time = 60;
        // uint16_t stop_time = 60; //seconds
        // enter_stop(stop_time);
        // standby_time += DATA_TS/1000 * (sch_config.total_ts - curr_data_ts);
        // sprintf(printf_buf, "going to sleep for %u seconds\n",  standby_time);
        //  HAL_UART_Transmit(&huart2, (uint8_t*)printf_buf, strlen(printf_buf), HAL_MAX_DELAY);
        //  enter_standby(hrtc, huart2,  standby_time);

    } else {
        for (int i = 0; i < sch_config.num_rx_ts; i++) {
            if (curr_data_ts == sch_config.rx_ts[i]){
                idle_ts = false;
                HAL_printf("receive slot: %hu\nreceiving...\n", sch_config.rx_ts[i]);

                raw_packet = rdt_recv_data_pkt(&huart1, &huart2, config, (DATA_TS - (LORA_TOA + TIME_BUF)));

                if(raw_packet != NULL){
                    recvd_packet = (mixnet_packet_t*)raw_packet;

                    HAL_printf("[%d] received DATA packet from node %d\n", config->node_addr, recvd_packet->src_address); 

                    recv_port = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
                    if(recv_port != -1 && (recvd_packet->type == PACKET_TYPE_DATA)) {
                        recvd_data_pkt = (packet_data_t*)recvd_packet->payload;
                        populate_sensor_data(sensor_db, recvd_data_pkt);
                    }
                    free(recvd_packet);
                }
            }
        }
    }

    if(idle_ts && (curr_data_ts != -1)) {
        HAL_printf("IDLE\n");
    }

    curr_data_ts++;
}

void detect_fire(mixnet_node_config_t *config, data_config_t *sensor_db, uint64_t timespan) {
    int16_t raw_temp = 0, 
            start_temp = 0, 
            end_temp = 0, 
            temp_incr = 0;    
    uint64_t start_time = tim2_tick_count;

    uint32_t sample_num = 0;
    int idx = 0;
    char printf_buf[40];

    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);

    int16_t warmup  = 1000;
        do{
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 1); // 1 ms timeout
            raw_temp = HAL_ADC_GetValue(&hadc1);
            }
        while(tim2_tick_count < (start_time + warmup));
        do{
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 1); // 1 ms timeout
            raw_temp = HAL_ADC_GetValue(&hadc1);
            if(sample_num == 0){
                start_temp = raw_temp;
                for (int i = 0; i < 9; i++)
                {
                    raw_temp = HAL_ADC_GetValue(&hadc1);
                    start_temp += raw_temp;
                }
                start_temp =  start_temp / 10;
                HAL_printf("sample %lu: %u\n", sample_num, start_temp);
                // wait_ms(PRINT_TIME);
            }
            sample_num++;
        }
        while(tim2_tick_count < (start_time + timespan));
        end_temp = raw_temp;
        for (int i = 0; i < 9; i++){
            raw_temp = HAL_ADC_GetValue(&hadc1);
            end_temp += raw_temp;
        }
        end_temp =  end_temp / 10;
    HAL_printf("sample %lu: %u\n", sample_num, raw_temp);
    // wait_ms(PRINT_TIME);

    temp_incr = abs(start_temp - end_temp);
    HAL_printf("Temperature increased by %d\n", temp_incr);
    // wait_ms(PRINT_TIME);

    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

    if(temp_incr >= TEMP_THRESH) {
        idx = sensor_db->num_nodes_on_fire;
        sensor_db->nodes_on_fire[idx] = config->node_addr;
        sensor_db->num_nodes_on_fire = sensor_db->num_nodes_on_fire + 1;
    }
}
