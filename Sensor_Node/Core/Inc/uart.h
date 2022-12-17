#ifndef _UART_H_
#define _UART_H_

char UART1_rx_byte();
void UART1_tx_byte(char byte);
void UART1_tx(char *data);
void UART1_rx(char *buffer, char delim, int num_bytes);

char UART2_rx_byte();
void UART2_tx_byte(char byte);
void UART2_tx(char *data);

#endif