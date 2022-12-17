/**
  * @file           : lora.c
  * @brief          : Transciever functions for communicating with RYLR896 module over UART, 
  *                   RPi UART initialization and setup using termios (see manpage for more info)
  *                   UART initializaiton code adapted from: https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/#basic-setup-in-c
  * @author         : Arden Diakhate-Palme, Geoferry Hunter 
  * @date           : 14.12.22
**/

// C library headers
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <sys/time.h>

#include "../inc/lora.h"
#include "../inc/gpio.h"

#define MAX_CMD_LEN 240

// Maximum packet size in bytes
#define MAX_PKT_SZ 240
#define MAX_PAYLOAD_SZ 212
#define MAX_RESP_LEN 8

// blocking RX/TX times (deci-seconds) for UART
#define RX_TIMEOUT 1

const int NET_PKT_SZ = 180;

// Create new termios struct, we call it 'tty' for convention
// No need for "= {0}" at the end as we'll immediately write the existing
// config to this struct
static struct termios tty;
static int serial_port;

void init_UART() {
    serial_port = open("/dev/serial0", O_RDWR | O_NOCTTY);

    // Check for errors
    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
    }

    // Read in existing settings, and handle any error
    // NOTE: This is important! POSIX states that the struct passed to tcsetattr()
    // must have been initialized with a call to tcgetattr() overwise behaviour
    // is undefined
    if(tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    // Set in/out baud rate to be 115200
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }
}

int read_resp(char *resp_buf, char *exp_resp){
    int exp_resp_len = strlen(exp_resp);
    char recv_buf[40];
    char printf_buf[60];
    int num_bytes_read;

    tty.c_cc[VTIME] = 30; 
    tty.c_cc[VMIN] = exp_resp_len;
    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }

    printf("response:\n");
    num_bytes_read = read(serial_port, recv_buf, exp_resp_len);
    snprintf(printf_buf, (exp_resp_len), "%s\n",recv_buf); //puts a \0 on last byte
    printf("%s\n", printf_buf);
    for(int i=0; i<num_bytes_read; i++){
        printf("%x ", recv_buf[i]);
    }
    printf("\n");

    tcflush(serial_port, TCIOFLUSH);

    recv_buf[exp_resp_len] = '\0';
    printf("Response [%d bytes] %s", exp_resp_len, recv_buf);
    printf("Expected [%d bytes] %s", exp_resp_len, recv_buf);

    return 0;
}

int LORA_cmd(uint8_t *in_cmd, uint16_t cmd_len, char *response) {
    uint8_t resp_len = strlen(response);
    if(strncmp((char*)in_cmd, "AT+RESET", 8) == 0){
        resp_len+=9;
    }

	uint8_t recv_buf[20];
    int ret_val = -1;
    int num_bytes_read = 0;
    // char flush_buf[MAX_CMD_LEN];

    char cmd[MAX_CMD_LEN];
    memcpy(cmd, in_cmd, cmd_len);
    cmd[cmd_len++] = '\r';
    cmd[cmd_len++] = '\n';

    write(serial_port, cmd, cmd_len);

    tty.c_cc[VTIME] = 30; 
    tty.c_cc[VMIN] = resp_len;
    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }

    // printf("response:\n");
    num_bytes_read = read(serial_port, recv_buf, resp_len);
    // snprintf(printf_buf, (resp_len), "%s\n",recv_buf); //puts a \0 on last byte
    // printf("%s\n", printf_buf);
    // for(int i=0; i<num_bytes_read; i++){
    //     printf("%x ", recv_buf[i]);
    // }
    // printf("\n");

    tcflush(serial_port, TCIOFLUSH);

    // Error checking
    if(strncmp((char*)in_cmd, "AT+RESET", 8) == 0){
        resp_len-=9;
    }

    recv_buf[resp_len] = '\0';
    // printf("Response [%d bytes] %s", resp_len, recv_buf);
    // printf("Expected [%d bytes] %s", resp_len, recv_buf);
    bool control_cmd = true;
    if(control_cmd) {
        if(strncmp((char*)recv_buf, response, resp_len) == 0){
            ret_val = 0;
        }else{
            printf("Command %s failed\n", cmd);
        } 
    }else{
        printf("Not a control command\n");
    }

    printf("\n");
    return ret_val;
}

int config_LORA(uint16_t address) {
    int ret_val= 0;

    char commands[1][40] = { 
    "AT+NETWORKID=6"
    };

    char addr_cmd[30];
    sprintf(addr_cmd, "AT+ADDRESS=%u", address);
    printf("%s\n", addr_cmd);

    ret_val |= LORA_cmd((uint8_t*)addr_cmd, strlen(addr_cmd), "+OK\r\n");

    for(int i=0; i<1; i++){
        printf("%s\n", commands[i]);
        ret_val |= LORA_cmd((uint8_t*)commands[i], strlen(commands[i]), "+OK\r\n");
    }
        
    return ret_val;
}

int init_LORA() {
    int ret_val= 0;

    char commands[3][40] = { 
        "AT+PARAMETER=10,9,1,7",
        "AT+BAND=927000000",
        "AT+CRFOP=15",
    };


    for(int i=0; i<3; i++){
        printf("%s\n", commands[i]);
        ret_val |= LORA_cmd((uint8_t*)commands[i], strlen(commands[i]), "+OK\r\n");
    }

    return ret_val;
}

int LORA_send_packet(mixnet_packet_t *pkt) {
    char *packet = malloc(MAX_PKT_SZ);
    char *filler = calloc(NET_PKT_SZ, 1);
    for(int i=0; i<NET_PKT_SZ; i++){
        filler[i] = 'A';
    }

    uint16_t packet_size = pkt->payload_size + sizeof(mixnet_packet_t);
    mixnet_address dst_addr = pkt->dst_address;
    int ret_val = 0;

    if(packet_size > MAX_PAYLOAD_SZ){
        printf("ERROR: Packet is too big!\n");
        return -1;
    }

    // NOTE: any packet sent must have a payload of less that NET_PKT_SZ bytes
    uint16_t bytes_written = (uint16_t)sprintf((char*)packet,"AT+SEND=%u,%u,", dst_addr, NET_PKT_SZ);
    uint16_t total_bytes_written = bytes_written + NET_PKT_SZ;
    printf(">>>> %u\n", bytes_written);
    memcpy(&packet[bytes_written], pkt, packet_size);
    assert((NET_PKT_SZ - packet_size) > 0);
    assert(total_bytes_written < MAX_PKT_SZ);
    memcpy((char*)(&packet[bytes_written]) + packet_size, filler, (NET_PKT_SZ - packet_size));

    set_green_led();
    while((ret_val = LORA_cmd((uint8_t*)packet, (NET_PKT_SZ + bytes_written), "+OK\r\n")) != 0){
        // printf("SEND CMD failed, retrying...\n");
    }
    clear_green_led();

    free(packet);
    return ret_val;
}

void print_bytes(char *arr, int arr_len){
    for(int i=0; i<arr_len; i++){
        printf("%x ", arr[i]);
    }
    printf("\n\n");

    for(int i=0; i<arr_len; i++){
        printf("%c ", arr[i]);
    }

    printf("\n\n");
}

int readline_LORA(char *recv_buf){
    tty.c_cc[VTIME] = 244; // in deciseconds
    tty.c_cc[VMIN] = 0; 

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }

    char byte, prev_byte;
    int recv_idx;
    bool lora_resp_end = false;
    int num_bytes_read = 0;
    int total_bytes_read = -1;
    int network_pkt_size = NET_PKT_SZ + 18;

    char tmp_buf[100];

    while(1){
        if(total_bytes_read >= network_pkt_size)
            break;

        num_bytes_read = 0;
        // printf("%d bytes left\n", (network_pkt_size - total_bytes_read));
        if((num_bytes_read=read(serial_port, tmp_buf, (network_pkt_size - num_bytes_read))) < 0){
            return -1;
        }
        
        byte = 0;
        prev_byte = 0;
        for(int i=0; i<num_bytes_read; i++){
            byte = tmp_buf[i];
            // printf("%02x ", byte);
            recv_buf[recv_idx++]=byte;

            if(byte == '\n' && prev_byte == '\r'){
                lora_resp_end = true;
                break;
            }
            prev_byte = byte;
        }

        total_bytes_read += num_bytes_read;

        if(lora_resp_end) 
            break;
    }


    return total_bytes_read;
}

char *lora_recv_data(int timeout, err_code_t *err_code) {
    uint8_t *recv_buf = calloc(MAX_PKT_SZ, 1);
    int packet_size;

    int num_bytes_read;
    int num_tok=0;
    char *packet = NULL;
    *err_code = PKT_ERROR;

    tty.c_cc[VTIME] = (timeout / 100); // in deciseconds
    tty.c_cc[VMIN] = 0; // just the beginning

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }

    struct timeval start_time, end_time;
    int elapsed_time = 0;
    uint8_t recvd_byte;

    gettimeofday(&start_time, NULL);
    do{
        (void)read(serial_port, &recvd_byte, 1);
        // printf("read %c\n", recvd_byte);
        gettimeofday(&end_time, NULL);
        elapsed_time = ((end_time.tv_sec * 1000 + (end_time.tv_usec / 1000)) - 
                        (start_time.tv_sec * 1000 + (start_time.tv_usec / 1000)));
    }while(elapsed_time < timeout && (recvd_byte != '+'));
    
    set_red_led();

    //if we didn't receive a packet in time allotted
    if(elapsed_time >= timeout){
        // printf("Did not receive a packet in %d ms\n", elapsed_time);
        free(recv_buf);
        clear_red_led();
        return NULL;
    }

    if((num_bytes_read=readline_LORA((char*)recv_buf)) < 0){
        printf("Readline error");
        return NULL;
    }

    /*
    printf("[received %d bytes]\n", num_bytes_read);

    for(int i=0; i<num_bytes_read; i++){
        printf("%x ", recv_buf[i]);
    }
    printf("\n");
    for(int i=0; i<num_bytes_read; i++){
        printf("%c ", recv_buf[i]);
    }
    printf("\n");
    */

    int resp_start_idx = -1;
    for(int i=0; i<num_bytes_read; i++){
        if((strncmp((char*)recv_buf, "+ERR", 4) == 0) || (strncmp((char*)recv_buf, "+RCV", 4) == 0)){
            resp_start_idx= i;
            break;
        }
    }

    if(resp_start_idx > 0){
        recv_buf = &recv_buf[resp_start_idx];
    }


    elapsed_time = ((end_time.tv_sec * 1000 + (end_time.tv_usec / 1000)) - (start_time.tv_sec * 1000 + (start_time.tv_usec / 1000)));
    // printf("Waited %d ms for a packet\n", elapsed_time);

    if((strncmp((char*)recv_buf, "ERR", 3) == 0) || (strncmp((char*)recv_buf, "+ERR", 4) == 0)){
        char tmp_buf[10];
        snprintf(tmp_buf, 6, "%s", (char*)recv_buf);
        printf("Error occured while receiving packet\n%s\n", tmp_buf);
        if(strncmp((char*)recv_buf, "ERR=12", 6) == 0){
            *err_code = PKT_ERROR_CRC;
        }else{
            *err_code = PKT_ERROR_UNK;
        }

    }else if((strncmp((char*)recv_buf, "RCV", 3) == 0) || (strncmp((char*)recv_buf, "+RCV", 4) == 0)){
        // +RCV=<Address>,<Length>,<Data>,<RSSI>,<SNR>,
        char *lora_ele;
        char *lora_ele_remainder = (char*)recv_buf;
        packet = malloc(MAX_PKT_SZ);
        while((lora_ele = strtok_r(lora_ele_remainder , ",", &lora_ele_remainder))) {
            // printf(lora_ele);
            // printf("\n");

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
        *err_code = NO_ERROR;
    }else{
        printf("ANOMALY:\n[received %d bytes]\n", num_bytes_read);

        for(int i=0; i<num_bytes_read; i++){
            printf("%x ", recv_buf[i]);
        }
        printf("\n");
        for(int i=0; i<num_bytes_read; i++){
            printf("%c ", recv_buf[i]);
        }
        printf("\n");
    }


    free(recv_buf);
    clear_red_led();
    return packet;
}

char *lora_recv_data_blocking(err_code_t *err_code) {
    uint8_t *recv_buf = calloc(MAX_PKT_SZ, 1);
    int packet_size;

    int num_bytes_read;
    int num_tok=0;
    char *packet = NULL;

    *err_code = PKT_ERROR;

    tty.c_cc[VTIME] = 1; // in deciseconds
    tty.c_cc[VMIN] = 1; 

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    }

    set_red_led();

    int network_pkt_size = 20 + NET_PKT_SZ;
    char byte, prev_byte;
    bool copying = false;
    for(int i=0; i<network_pkt_size ; i++){
        if((num_bytes_read = read(serial_port, &byte, 1)) < 0){
            fprintf(stderr, "read error\n");
            free(recv_buf);
            clear_red_led();
            return NULL;
        }
        if(byte == '+'){
            copying = true;
        }
        if(copying && prev_byte == '\r' && byte == '\n'){
            num_bytes_read = i;
            break;
        }
        if(copying){
            recv_buf[i]=byte;
            prev_byte = byte;
        }
    }

    printf("[received %d bytes]\n", num_bytes_read);

    for(int i=0; i<num_bytes_read; i++){
        printf("%x ", recv_buf[i]);
    }
    printf("\n");
    for(int i=0; i<num_bytes_read; i++){
        printf("%c ", recv_buf[i]);
    }
    printf("\n");


    // int cmd_start_idx = 0;
    for(int i=0; i<num_bytes_read; i++){
        if((strncmp((char*)&recv_buf[i], "+ERR", 4) == 0) || (strncmp((char*)&recv_buf[i], "ERR", 3) == 0) ||
           (strncmp((char*)&recv_buf[i], "+RCV", 4) == 0) || (strncmp((char*)&recv_buf[i], "RCV", 3) == 0)){
            recv_buf = (uint8_t*)&recv_buf[i];
            break;
        }
    }

    if((strncmp((char*)recv_buf, "+ERR", 4) == 0) || (strncmp((char*)recv_buf, "ERR", 3) == 0)){
        char tmp_buf[10];
        snprintf(tmp_buf, 6, "%s", (char*)recv_buf);
        printf("Error occured while receiving packet\n%s\n", tmp_buf);

    }else if((strncmp((char*)recv_buf, "+RCV", 4) == 0) || (strncmp((char*)recv_buf, "RCV", 3) == 0)){
        // +RCV=<Address>,<Length>,<Data>,<RSSI>,<SNR>,
        char *lora_ele;
        char *lora_ele_remainder = (char*)recv_buf;
        packet = malloc(MAX_PKT_SZ);
        while((lora_ele = strtok_r(lora_ele_remainder , ",", &lora_ele_remainder))) {
            printf(lora_ele);
            printf("\n");

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

        *err_code = NO_ERROR;

    }else{
        printf("ANOMALY:\n[received %d bytes]\n", num_bytes_read);

        for(int i=0; i<num_bytes_read; i++){
            printf("%x ", recv_buf[i]);
        }
        printf("\n");
        for(int i=0; i<num_bytes_read; i++){
            printf("%c ", recv_buf[i]);
        }
        printf("\n");
    }


    free(recv_buf);
    clear_red_led();
    return packet;
}
