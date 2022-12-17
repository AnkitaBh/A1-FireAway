#include "stm32l4xx.h"
#include "stm32l412xx.h"

#include "stm32l4xx_hal.h"


#define UART_TXE (1 << 7)
#define UART_RXNE (1 << 7)

char UART1_rx_byte();
void UART1_tx_byte(char byte);

char UART2_rx_byte();
void UART2_tx_byte(char byte);

void UART1_tx_byte(char byte) {
    USART_TypeDef *uart= USART1;
    while(!(uart->ISR & UART_TXE));
    uart->TDR = byte;
    return;
}

char UART1_rx_byte() {
    USART_TypeDef *uart= USART1;
    while(!(uart->ISR & UART_RXNE));
    return (char)uart->RDR;
}

void UART2_tx_byte(char byte) {
    USART_TypeDef *uart= USART2;
    while(!(uart->ISR & UART_TXE));
    uart->TDR = byte;
    return;
}

char UART2_rx_byte() {
    USART_TypeDef *uart= USART2;
    while(!(uart->ISR & UART_RXNE));
    return (char)uart->RDR;
}

void UART2_tx(char *data) {
    int i=0;
    char byte;
    while(byte != '\n' && byte != 0) {
        byte= data[i];
        UART2_tx_byte(byte);
        i++;
    }
}

void UART1_tx(char *data) {
    int i=0;
    char byte;
    while(byte != '\n') {
        byte= data[i];
        UART1_tx_byte(byte);
        i++;
    }
}

void UART1_rx(char *buffer, char delim, int num_bytes) {
    char byte;
    int i=0;
    while(byte != delim && i < num_bytes){
        byte = UART1_rx_byte();
        buffer[i] = byte;
        i++;
    }
}

void UART2_rx(char *buffer, char delim, int num_bytes) {
    char byte;
    int i=0;
    while(byte != delim && i < num_bytes){
        byte = UART2_rx_byte();
        buffer[i] = byte;
        i++;
    }
}
