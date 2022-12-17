/**
  * @file           : lora.h
  * @brief          : Definitions for transciever functions for communicating with RYLR896 module over UART, 
  * @author         : Arden Diakhate-Palme 
  * @date           : 14.12.22
**/
#ifndef _LORA_H_
#define _LORA_H_

#include "packet.h"

enum err_codes {NO_ERROR = 0, PKT_ERROR, PKT_ERROR_CRC, PKT_ERROR_UNK};
typedef uint8_t err_code_t;

// settings not stored in EEPROM, required every initialization
void init_UART();

// required once per LoRa
int config_LORA(uint16_t address);
int init_LORA();

int readline_LORA(char *recv_buf);
int LORA_cmd(uint8_t *in_cmd, uint16_t cmd_len, char *response);
int LORA_send_cmd(uint8_t *packet, uint16_t packet_size, char *response);
int LORA_send_packet(mixnet_packet_t *pkt);

char *lora_recv_data(int timeout, err_code_t *err_code);
char *lora_recv_data_blocking(err_code_t *err_code);
int read_resp(char *resp_buf, char *exp_resp);

#endif