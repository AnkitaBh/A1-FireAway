#ifndef _LORA_H_
#define _LORA_H_

#include "stm32l4xx.h"
#include <stdbool.h>
#include "packet.h"
#include "prints.h"

#define DEBUG_LORA 1

#define MAX_CMD_LEN 30

// Maximum packet size in bytes
#define MAX_PAYLOAD_SZ 212
#define MAX_RESP_LEN 8

// blocking RX/TX times (ms) for UART
#define RX_TIMEOUT 1000
#define TX_TIMEOUT 100


typedef struct {
    uint8_t byte;
    uint8_t prev_byte;
    int msg_len;
    bool msg_end;
    bool echo_resp;
    bool copying_bytes;
    char buf[240];
    int buf_idx;
} lora_info_t;

enum recv_mode {BLOCKING=0, NON_BLOCKING};
typedef uint8_t recv_mode_t;

enum err_codes {NO_ERROR = 0, PKT_ERROR, PKT_ERROR_CRC, PKT_ERROR_UNK};
typedef uint8_t err_code_t;

/* Configuration functions and helpers */
int LORA_cmd(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, uint8_t *in_cmd, uint8_t cmd_len, char *response, int rx_timeout, int tx_timeout);
int config_LORA(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, uint16_t address);
int init_LORA(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart);
int sleep_LORA();

int LORA_send_packet(UART_HandleTypeDef *lora_huart, UART_HandleTypeDef *serial_huart, mixnet_packet_t *pkt);
char *lora_recv_data(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, 
                        uint64_t start_time, uint64_t timeout, recv_mode_t recv_mode, err_code_t *err_code);

#endif
