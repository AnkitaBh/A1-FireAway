/**
  * @file           : lora.c
  * @brief          : Transciever functions for communicating with RYLR896 module over UART,
  *                   and (non)-blocking packet receive calls & packet sending functions
  * @author         : Arden Diakhate-Palme
  *                   Modified by karenabruzzo 
  * @date           : 14.12.22
**/

#include "lora.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "uart.h"
#include "main.h"
#include "packet.h"
#include "prints.h"

extern int BYTE_RX_DELAY; 
extern int PKT_TX_DELAY;

extern uint32_t tim2_tick_count;
extern int MAX_PKT_SZ;
const int NET_PKT_SZ = 180;

extern int PRINT_TIME;
extern int LORA_TOA;

extern UART_HandleTypeDef huart1; // Lora UART
extern UART_HandleTypeDef huart2; // Debug UART

extern lora_info_t lora;



int lin_search(char *arr, int arr_len, char *search_str, int search_str_len) {
    int res=-1;
    for(int i=0; i<arr_len; i++){
        if(memcpy(&arr[i], search_str, search_str_len) == 0){
            res = i;
            break;
        }
    }

    return res;
}

void print_bytes(char *arr, int arr_len){
    char printf_buf[10];
    for(int i=0; i<arr_len; i++){
        HAL_printf("%x ", arr[i]);
    }
    HAL_printf("\n\n");


    for(int i=0; i<arr_len; i++){
        HAL_printf("%c ", arr[i]);
    }

    HAL_printf("\n\n");
}

int readline_LORA(char *recv_buf, recv_mode_t recv_mode, uint32_t timeout) {
    int recvd_msg_len = -1;
    uint32_t start_time = tim2_tick_count;

    lora.copying_bytes = false;
    lora.msg_end = false;
    lora.buf_idx = 0;
    lora.msg_len = 0;

  	HAL_UART_Receive_IT(&huart1, &lora.byte, 1);

    if(recv_mode == BLOCKING){
        while(!lora.msg_end);

    }else if(recv_mode == NON_BLOCKING){
        while((tim2_tick_count - start_time) < timeout){
            if(lora.msg_end) break;
        }
    }

    // messages are defined as +..\r\n
    if(lora.msg_end && lora.copying_bytes){
        lora.buf[lora.msg_len] = '\0';
        HAL_printf("%s", lora.buf);

        memcpy(recv_buf, lora.buf, lora.msg_len);
        recvd_msg_len = lora.msg_len;
    }

    return recvd_msg_len;
}

int LORA_cmd(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, 
            uint8_t *in_cmd, uint8_t cmd_len, char *response, int rx_timeout, int tx_timeout) {
    int resp_len = 0;
    uint8_t exp_resp_len = strlen(response);
	char recv_buf[20];
    char printf_buf[MAX_PKT_SZ];
    int ret_val = -1;

    uint8_t cmd[MAX_PKT_SZ];
    memcpy(cmd, in_cmd, cmd_len);
    cmd[cmd_len++] = '\r';
    cmd[cmd_len++] = '\n';

    // snprintf(printf_buf, 40, "sending %u bytes: %s\n", cmd_len, cmd);
    // HAL_printf("%s", printf_buf);

    HAL_UART_Transmit_IT(&huart1, (uint8_t*)cmd, cmd_len);

    uint64_t start_time = tim2_tick_count;
    
    if((strncmp((char*)in_cmd, "AT+RESET", 8) == 0) ){
        resp_len = readline_LORA(recv_buf, NON_BLOCKING, rx_timeout);
        if((strncmp((char*)recv_buf, response, 6) == 0) && (strncmp((char*)&recv_buf[9], &response[9], 6) == 0)) {
            ret_val = 0;
        }
        HAL_printf("response: %s\n", recv_buf);
        // wait_ms(PRINT_TIME);
    }else if(strncmp((char*)in_cmd, "AT+ADDRESS", 10) == 0){
        resp_len = readline_LORA(recv_buf, NON_BLOCKING, rx_timeout);
        if((resp_len != -1) && strncmp((char*)recv_buf, response, resp_len) == 0) {
            ret_val = 0;
        }
        recv_buf[3] = '\0';
        HAL_printf("response: %s\n", recv_buf);
        // wait_ms(PRINT_TIME);

    }else if(strncmp((char*)in_cmd, "AT+SEND", 7) == 0) {
        resp_len = readline_LORA(recv_buf, BLOCKING, 0);

        char lora_msg[20];
        int lora_msg_len = 0;
        int idx=0;
        for(int i=0; i < resp_len; i++){
            if(memcmp(&recv_buf[i], "+OK\r\n", 5) == 0){
                memcpy(lora_msg, &recv_buf[i], 5);
                lora_msg_len = 5;
                ret_val = 0;
                break;

            } else if((memcmp(&recv_buf[i], "+ERR", 4) == 0)) {
                break; 
                if(strncmp(&recv_buf[i], "+ERR=2", 5) == 0){
                    HAL_printf("must reset now!\n");
                    HAL_UART_Transmit_IT(lora_huart, "AT+RESET\r\n", 10);
                    HAL_UART_Receive_IT(lora_huart, &lora.byte, 1);
                    wait_ms(1000);
                }
            }
        }
        // print_bytes(lora_msg, lora_msg_len);
        // in_cmd[8] = '\0';
        // recv_buf[3] = '\0';
        // HAL_printf("%s...%s\n", in_cmd, recv_buf);

    }else{
        resp_len = readline_LORA(recv_buf, BLOCKING, 0);
        if((resp_len != -1) && strncmp((char*)recv_buf, response, resp_len) == 0) {
            ret_val = 0;
        }
        recv_buf[3] = '\0';
        HAL_printf("%s\n", recv_buf);
        // wait_ms(PRINT_TIME);
    }

    return ret_val;
}

int config_LORA(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, uint16_t address) {
    int ret_val= 0, resp_len=0;
    char addr_cmd[30];
    char recv_buf[30];
    char ready_buf[30];
    sprintf(addr_cmd, "AT+ADDRESS=%u", address);

    HAL_printf("AT+RESET\n");
    HAL_UART_Transmit_IT(lora_huart, "AT+RESET\r\n", 10);
    HAL_UART_Receive_IT(lora_huart, &lora.byte, 1);
    wait_ms(1000);

    HAL_printf("%s\n",addr_cmd);
    while((ret_val=LORA_cmd(lora_huart, serial_huart, addr_cmd, strlen(addr_cmd), "+OK\r\n", 1000, 1000)) < 0){
        wait_ms(10);
    }

        
    return ret_val;
}

int sleep_LORA(){
    char sleep_cmd[30] = "AT+MODE=1";
    HAL_printf("%s\n",sleep_cmd);
    LORA_cmd(&huart1, &huart2, (uint8_t*)sleep_cmd, strlen(sleep_cmd), "+OK\r\n", 100, 100);
    return 0;
}

int init_LORA(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart) {
    int ret_val= 0;
    int num_cmds = 4;

    char cmds[4][MAX_CMD_LEN] = { 
        "AT+PARAMETER=10,9,1,7",
        "AT+BAND=927000000",
        "AT+CRFOP=15",
        "AT+MODE=0",
    };

    // settings NOT stored in EEPROM, required every initialization
    // ret_val |= LORA_cmd(lora_huart, serial_huart, (uint8_t*)"AT+RESET", 8, "+RESET\r\n +READY\r\n", 2000, 100);
    for(int i=0; i<num_cmds; i++){
        HAL_printf("%s\n",cmds[i]);
        ret_val |= LORA_cmd(lora_huart, serial_huart, (uint8_t*)cmds[i], strlen(cmds[i]), "+OK\r\n", 100, 100);
    }

    return ret_val;
}

int LORA_send_packet(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, mixnet_packet_t *pkt) {
    char printf_buf[40];
	uint8_t *packet = malloc(MAX_PKT_SZ);
    char *filler = calloc(NET_PKT_SZ, 1);
    for(int i=0; i<NET_PKT_SZ; i++){
        filler[i] = 'A';
    }

    uint16_t packet_size = pkt->payload_size + sizeof(mixnet_packet_t);
    mixnet_address dst_addr = pkt->dst_address;
    int ret_val= -1;

    if(packet_size > MAX_PAYLOAD_SZ){
        HAL_printf("packet too big!\n");
        return -1;
    }

    // any packet sent must have a payload of less that NET_PKT_SZ bytes
    uint16_t bytes_written = (uint16_t)sprintf((char*)packet,"AT+SEND=%u,%u,", dst_addr, NET_PKT_SZ);
    memcpy(&packet[bytes_written], pkt, packet_size);
    assert((NET_PKT_SZ - packet_size) > 0);
    memcpy((char*)(&packet[bytes_written]) + packet_size, filler, (NET_PKT_SZ - packet_size));

    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);

    // Blocks until we get a +OK (however long that takes)
    while((ret_val=LORA_cmd(lora_huart, serial_huart, packet, (NET_PKT_SZ + bytes_written), "+OK\r\n", 0, 0)) < 0){
        wait_ms(10);
    }

    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

    free(packet);
    return ret_val;
}


// can handle channel errors here, they are detected and 
// packets are re-transmitted in all phases after STP; for STP, we do not recover from errors such as CRC errors
char *lora_recv_data(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, 
                    uint64_t start_time, uint64_t timeout, recv_mode_t recv_mode, err_code_t *err_code) {
    char recv_buf[MAX_PKT_SZ];
    char lora_msg[MAX_PKT_SZ];
    int lora_msg_len = 0;
    int recvd_msg_size = 0;
    int elapsed_time = 0;

    int packet_size;
    char printf_buf[40];

    int num_tok=0;
    char *packet = NULL; 
    *err_code = PKT_ERROR;
    (void)start_time;

    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);

    // must check that a \r\n are received at the end of +RCV=...\r\n
    int lora_pkt_msg_len = 20 + NET_PKT_SZ;
    if((recvd_msg_size = readline_LORA(recv_buf, recv_mode, timeout)) < 0){
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
        *err_code = NO_ERROR;
        return NULL;
    }

    for(int i=0; i < recvd_msg_size; i++){
        if(memcmp(&recv_buf[i], "+RCV", 4) == 0){
            memcpy(lora_msg, &recv_buf[i], lora_pkt_msg_len);
            lora_msg_len = lora_pkt_msg_len;
            break;

        } else if((memcmp(&recv_buf[i], "+ERR", 4) == 0)) {
            HAL_printf("%s",recv_buf);
            HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

            if(memcmp(&recv_buf[i], "+ERR=12", 7) == 0){
                *err_code = PKT_ERROR_CRC;
            }else{
                *err_code = PKT_ERROR_UNK;
            }

            return NULL;
        }
    }

    // print_bytes(lora_msg, lora_msg_len);

    if((strncmp((char*)lora_msg, "+RCV", 4) == 0)){
        char *lora_ele;
        char *lora_ele_remainder = (char*)lora_msg;
        packet = malloc(sizeof(char) * MAX_PKT_SZ);
        while((lora_ele = strtok_r(lora_ele_remainder , ",", &lora_ele_remainder))) {

            if(num_tok == 1) {
                packet_size = atoi(lora_ele);
                break;
            }

            num_tok++;    
        }
        (void)packet_size;

        // copy the header
        mixnet_packet_t *pkt;
        memcpy(packet, lora_ele_remainder, sizeof(mixnet_packet_t));
        pkt = (mixnet_packet_t*)packet;

        // copy payload size bytes
        memcpy((char*)packet + sizeof(mixnet_packet_t), (char*)lora_ele_remainder + sizeof(mixnet_packet_t), pkt->payload_size);

        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

        *err_code = NO_ERROR;
        /*TESTING begin*/
        // if(recv_mode == NON_BLOCKING){
        //     *err_code = PKT_ERROR;
        //     return NULL;
        // }
        /*TESTING end */
    }else{
        HAL_printf("received something else: %s\n", recv_buf);
    } 

    return packet;
}
