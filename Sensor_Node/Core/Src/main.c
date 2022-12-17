/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * @author         : Arden Diakhate-Palme
  *                 Modified by Karen A., Ankita B.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include "lora.h"
#include "uart.h"
#include "protocols.h"
#include "packet.h"
#include "graph.h"
#include "schedule.h"
#include "exe_sch.h"
#include "stop.h"
#include "standby.h"
#include "prints.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/***** CONFIGURATION: change before uploading to each board ***/
const int NUM_NODES = 9;
const enum topology_t NETWORK_TOPOLOGY = MESH_9;
#define RESET_NODE 1   // NOTE: only resets address
#define TEST_MODEM 1   
const int GW_ROOT_ADDR = 1;
const int NODE_ADDR = 2;
/***** END CONFIGURATION ***/

const int MAX_STP_ITER = 1;
const int STP_TS = 4000; // in milliseconds
const int STP_CYCLE = (NUM_NODES + 1) * STP_TS;
const int MAX_PKT_SZ = 240;
const int DATA_TS = 18000; //in milliseconds
const int DC_TS = 5000; // in milliseconds
const int TEMP_THRESH = 85; // increase in ADC reading of temp to trigger a fire warning
const int DATA_RECV_WIN = 2000; // in milliseconds
const uint32_t ACK_TIMEOUT = 4000; // in milliseconds
const int PKT_TX_DELAY = 41; //ms

const int LORA_TOA = 1972; // Time on Air in ms
const int TIME_BUF = 300;
const int PRINT_TIME = 40; //ms

lora_info_t lora = {};

static bool ts_interrupt = false;

bool data_phase = false;
uint32_t tim2_tick_count = 0; // tick count in milliseconds

char lora_echo_buf[240];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM16_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_RTC_Init();
  MX_TIM16_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
    char printf_buf[80];
    int err=0;
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_Base_Start_IT(&htim16); 
    uint32_t start_time, end_time, elapsed_time;
    srand(time(NULL));

    // configure initial mesh topology
    mixnet_node_config_t config;
    config.node_addr = NODE_ADDR;
    set_topology(&config, NUM_NODES, NETWORK_TOPOLOGY);
    
    /* STP variables */
    int initial_stp_timeout = config.node_addr * STP_TS; // initial timeout to receive updates before transmitting your STP advertisement
    int stp_timeout, stp_remaining_time;
    int stp_iter = 0;
    stp_route_t stp_route_db;
    stp_route_db.root_address = config.node_addr;
    stp_route_db.path_length = 0;
    stp_route_db.next_hop_address = config.node_addr;
    stp_route_db.stp_ports = calloc(config.num_neighbors, 1);
    activate_all_ports(stp_route_db.stp_ports, config.num_neighbors);
    stp_route_db.stp_parent_addr = -1;
    stp_route_db.stp_parent_path_length = -1;
    mixnet_packet_stp_t *recvd_stp_pkt;
    bool stp_txen = false;              // whether the node will transmit in the half-cycle of the STP
    uint8_t *recvd_from_ports = calloc(config.num_neighbors, 1);

    /* LSA variables */
    uint8_t parent_port = 0;
    uint8_t num_tree_children = 0; 
    mixnet_packet_lsa_t *recvd_lsa_pkt;
    graph_t *net_graph = graph_init();
    (void)graph_add_neighbors(net_graph, config.node_addr, config.neighbor_addrs, config.num_neighbors);

    /* SCH variables */
    schedule_t node_sch;
    data_sch_config_t sch_config;
    packet_sch_t *recvd_sch_pkt;
    init_sch(&node_sch);
    init_sch_config(&sch_config);
    uint64_t delay_to_phase4 = 0;

    /* DATA variables */
    data_config_t sensor_db;
    packet_data_t *recvd_data_pkt;
    init_sensor_db(&config, &sensor_db);

    HAL_printf("[Node %d]\n", config.node_addr);

    // initialize transcievers
    start_time = tim2_tick_count;
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
    #if RESET_NODE
    if((err=config_LORA(&huart1, &huart2, config.node_addr))<0){
        HAL_printf("Configuration error\n");
        exit(1);
    }    
    HAL_printf("[configuration done]\n");
    wait_ms(PRINT_TIME);
    #endif
    wait_ms(PRINT_TIME);
    if((err=init_LORA(&huart1, &huart2))<0){
        HAL_printf("Initialization error\n");
        exit(1);
    }
    end_time = tim2_tick_count;
    HAL_printf("[initialization done]\n");
    wait_ms(PRINT_TIME);
    HAL_printf("time: %u\n", tim2_tick_count);
    wait_ms(PRINT_TIME);
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

    char *raw_packet;
    mixnet_packet_t *recvd_packet;
    int recv_port = 0;
    err_code_t err_code;
    bool goto_next_proto_iter = false;
    
    /* Testing reliable data transfer, and channel conditions */
    #if TEST_MODEM
    int iter = 1;
    start_time = tim2_tick_count;

    while(1){
        if(config.node_addr == 2) {
            HAL_printf("[%d] Receiving 3 packets..\n", config.node_addr);
            for(int i=0; i<3; i++) {
                raw_packet = rdt_recv_pkt(&huart1, &huart2, &config);
                assert(raw_packet != NULL);
                recvd_packet = (mixnet_packet_t*)raw_packet;
                HAL_printf("Application received packet [SEQ %u] from %u\n", recvd_packet->seqnum, recvd_packet->src_address);
            }

            wait_ms(1000);

            HAL_printf("[%d] Sending 3 packets...\n", config.node_addr);
            for(int i=0; i<3; i++){
                send_lsa_req(&config, 1);
            }
        }

        if(config.node_addr == 1) {
            HAL_printf("[%d] Sending 3 packets...\n", config.node_addr);
            for(int i=0; i<3; i++){
                send_lsa_req(&config, 2);
            }

            wait_ms(1000);

            HAL_printf("[%d] Receiving 3 packets..\n", config.node_addr);
            for(int i=0; i<3; i++) {
                raw_packet = rdt_recv_pkt(&huart1, &huart2, &config);
                assert(raw_packet != NULL);
                recvd_packet = (mixnet_packet_t*)raw_packet;
                HAL_printf("Application received packet [SEQ %u] from %u\n", recvd_packet->seqnum, recvd_packet->src_address);
            }
        }

        elapsed_time = tim2_tick_count - start_time;
        HAL_printf("[%d] (%u) cycle done in %lu ms\n\n", iter++, tim2_tick_count, elapsed_time);

        for(int i=0; i<iter; i++){
            HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
            wait_ms(500);
            HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
        }

        iter++;
    }
    #endif

    /* Synchronization Option 1:
     * Entry point for all nodes
     * Wait for X minutes in a lower power mode
     */
    
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) == RESET) {
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);  // clear the flag
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
        wait_ms(90000);
        /*
        uint16_t stop_time = 90; 
        enter_stop(stop_time);
        */
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    }

    /* TDM STP PROTOCOL */
    HAL_printf("============= Phase 1: STP =============\n");
    // wait_ms(PRINT_TIME);
    start_time = 0;
    end_time = 0;

    // set the indicate the root at onset (shine red LED)
    if(stp_route_db.root_address == GW_ROOT_ADDR) {
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
    }
    while(stp_iter < 2 * MAX_STP_ITER) {
        if(stp_iter % 2 == 0) {
            stp_txen = true;
            stp_timeout = initial_stp_timeout;
        }else{
            stp_txen = false;
            stp_timeout = STP_CYCLE - initial_stp_timeout;
        }

        end_time = tim2_tick_count;
        start_time = tim2_tick_count;
        do {
            // THIS PRINT IS NECESSARY
            HAL_printf("[iteration %u, timeout: %lu]\n", stp_iter+1, (uint32_t)(stp_timeout - (end_time - start_time)));

            raw_packet = lora_recv_data(&huart1, &huart2, start_time, (stp_timeout - (end_time - start_time)), NON_BLOCKING, &err_code);
            end_time = tim2_tick_count;

            // if we actually received a packet 
            if(raw_packet != NULL) {
                recvd_packet = (mixnet_packet_t*)raw_packet;

                #if DBG_STP
                HAL_printf("[%d] received packet type %u from %u\n", config.node_addr, recvd_packet->type, recvd_packet->src_address);
                wait_ms(PRINT_TIM);
                #endif
                
                // was the packet from a neighbor?
                recv_port = get_port(config.neighbor_addrs, config.num_neighbors, recvd_packet->src_address);
                if(recv_port != -1 && (recvd_packet->type == PACKET_TYPE_STP)) {
                    recvd_stp_pkt = (mixnet_packet_stp_t*)recvd_packet->payload;

                    HAL_printf("[%d] received STP packet: (%d, %d, %d)\n", 
                                config.node_addr, recvd_stp_pkt->root_address, recvd_stp_pkt->path_length, recvd_stp_pkt->node_address);
                    // wait_ms(PRINT_TIME);
                    UART2_tx(printf_buf);

                    recvd_from_ports[recv_port] = 1;

                    update_stp_db(&config, (uint8_t)recv_port, &stp_route_db, recvd_stp_pkt);
                    if(stp_route_db.root_address == GW_ROOT_ADDR){
                    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
                    }
                }

                free(recvd_packet);
            }
        }while((end_time - start_time) < stp_timeout);

        if(stp_txen) {
            broadcast_stp(&config, &stp_route_db);
        }

        stp_iter++;
    }

    // block ports to unresponsive neighbors (offline nodes) 
    for(int i=0; i<config.num_neighbors; i++) {
        stp_route_db.stp_ports[i] &= recvd_from_ports[i];
        if(recvd_from_ports[i] == 0){
            // remove from your own list (before even running LSA)
            remove_tree_neighbor(&config, net_graph, config.neighbor_addrs[i]);
        }
    }
    print_ports(&config, stp_route_db.stp_ports);
    print_graph(net_graph);
    
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

    /* LSA PROTOCOL */
    HAL_printf("============= Phase 2: LSA =============\n");
    // wait_ms(PRINT_TIME);

    if(config.node_addr == GW_ROOT_ADDR) {
        num_tree_children = get_child_node_lsa(&config, &stp_route_db, net_graph);

    // non-gateway nodes
    }else{
        raw_packet = rdt_recv_pkt(&huart1, &huart2, &config);
        
        // if we actually received a packet 
        if(raw_packet != NULL) {
            recvd_packet = (mixnet_packet_t*)raw_packet;

            #if DBG_LSA
            HAL_printf("[%d] received packet from node %u\n", config.node_addr, recvd_packet->src_address); 
            #endif
            
            // was the packet from a neighbor?
            recv_port = get_port(config.neighbor_addrs, config.num_neighbors, recvd_packet->src_address);
            if(recv_port != -1 &&  (recvd_packet->type == PACKET_TYPE_LSA)) {
                recvd_lsa_pkt = (mixnet_packet_lsa_t*)recvd_packet->payload;

                // was the packet from a TREE neighbor?
                if(stp_route_db.stp_ports[recv_port]) {
                    if(recvd_lsa_pkt->flags == LSA_REQ){
                        #if DBG_LSA
                        HAL_printf("[%d] received LSA request from %u\n", config.node_addr, recvd_packet->src_address);
                        #endif

                        parent_port = get_port(config.neighbor_addrs, config.num_neighbors, stp_route_db.stp_parent_addr);
                        assert(parent_port == recv_port);

                        stp_route_db.stp_ports[parent_port] = 0;
                        num_tree_children = get_child_node_lsa(&config, &stp_route_db, net_graph);
                        stp_route_db.stp_ports[parent_port] = 1;

                        wait_ms(500);

                        if((err=send_lsa_resp(&config, &stp_route_db, net_graph)) < 0){
                            HAL_printf("[%d] Failed to send LSA_RESP to upstream parent\n", config.node_addr);
                            goto_next_proto_iter = true;
                        }

                        HAL_printf("[%d] sent complete LSA_RESP\n", config.node_addr);

                    }else{
                        HAL_printf("[%d] ERROR: expected LSA_REQ, received %u\n", config.node_addr, recvd_lsa_pkt->flags); 
                    }

                // packet not from a TREE neighbor 
                }else{
                    if(recvd_lsa_pkt->flags == LSA_REQ){
                        HAL_printf("[%d] got LSA_REQ from inactive link\n", config.node_addr); 
                        send_lsa_inv(&config, recvd_packet->src_address);
                    }
                }
            }
            free(recvd_packet);
        }
    }

    if(goto_next_proto_iter){
        HAL_printf("[%d] Waiting in standby until the next time the protocol is run\n", config.node_addr); 
        while(1);
    }

    print_ports(&config, stp_route_db.stp_ports);
    print_graph(net_graph);

    /* RX/TX SCHEDULE */
    HAL_printf("============= Phase 3: SCH =============\n");

    if(config.node_addr == GW_ROOT_ADDR) {
        populate_schedule(net_graph, &node_sch);
        print_schedule(&node_sch);

        // send schedule to each neighbor, and wait for an ACK from them 
        query_sch(&config, &stp_route_db, &node_sch, delay_to_phase4);
    
    // non-gateway nodes
    }else{
        while(1) {
            raw_packet = rdt_recv_pkt(&huart1, &huart2, &config);
            
            // if we actually received a packet 
            if(raw_packet != NULL) {
            recvd_packet = (mixnet_packet_t*)raw_packet;

                // was the packet from a neighbor?
                recv_port = get_port(config.neighbor_addrs, config.num_neighbors, recvd_packet->src_address);
                if(recv_port != -1){
                    if((recvd_packet->type == PACKET_TYPE_SCH) && (stp_route_db.stp_ports[recv_port] == 1)) {
                        recvd_sch_pkt = (packet_sch_t*)recvd_packet->payload;
                        
                        HAL_printf("[%d] received schedule from node %u\n", config.node_addr, recvd_packet->src_address);
                        
                        delay_to_phase4 = recvd_sch_pkt->delay_to_phase4;
                        populate_local_sch(&node_sch, recvd_sch_pkt);
                        print_schedule(&node_sch);

                        populate_sch_config(&config, &node_sch, &sch_config);

                        // send schedule to each child, and wait for an ACK from each of them 
                        parent_port = get_port(config.neighbor_addrs, config.num_neighbors, stp_route_db.stp_parent_addr);
                        assert(parent_port == recv_port);

                        if(num_tree_children > 0) {
                            stp_route_db.stp_ports[parent_port] = 0;
                            query_sch(&config, &stp_route_db, &node_sch, delay_to_phase4);
                            stp_route_db.stp_ports[parent_port] = 1;
                        }

                        wait_ms(500);

                        send_sch_ack(&config, stp_route_db.stp_parent_addr);

                        HAL_printf("[%d] sent SCH ACK to node %u\n", config.node_addr, stp_route_db.stp_parent_addr);
                        break;

                    }else if((recvd_packet->type == PACKET_TYPE_LSA) &&
                            (stp_route_db.stp_ports[recv_port] == 0) &&
                            (recvd_lsa_pkt->flags == LSA_REQ)){
                        HAL_printf("[%d] got LSA_REQ from inactive link\n", config.node_addr); 
                        send_lsa_inv(&config, recvd_packet->src_address);
                    }
                }
                
                free(recvd_packet);
            }
        }
        print_sch_config(&sch_config);
    }

    HAL_printf(printf_buf, "time: %lu, waiting %lu\n", (uint32_t)tim2_tick_count, (uint32_t)(delay_to_phase4 - tim2_tick_count)); //depends on the topology
    if(delay_to_phase4 > tim2_tick_count){
        wait_ms(delay_to_phase4 - tim2_tick_count);
    }

    /* DATA Phase */
    HAL_printf("============= Phase 4: DATA ============\n");

    if(config.node_addr == GW_ROOT_ADDR){
        HAL_printf("Running echo server\n");

        raw_packet = rdt_recv_pkt(&huart1, &huart2, &config);

        if(raw_packet != NULL){
            recvd_packet = (mixnet_packet_t*)raw_packet;

            recv_port = get_port(config.neighbor_addrs, config.num_neighbors, recvd_packet->src_address);
            if(recv_port != -1 && (recvd_packet->type == PACKET_TYPE_DATA)) {
                recvd_data_pkt = (packet_data_t*)recvd_packet->payload;
                populate_sensor_data(&sensor_db, recvd_data_pkt);
            }
            free(recvd_packet);
        }

        print_sensor_db(&sensor_db);

    }else{
        exe_sch();

        while(data_phase){
            if(ts_interrupt){
                check_data_schedule(&config, stp_route_db.stp_parent_addr, sch_config, &sensor_db);
                ts_interrupt = false;
            }
        }

        HAL_printf("Done with protocol stack.\n");
    }

    int16_t standby_time = 60;
    HAL_printf("going to sleep for %u seconds\n",  standby_time);
    sleep_LORA();
    enter_standby(hrtc, huart2,  standby_time);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 32 - 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000 - 1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 32000-1;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 1000-1;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */
  
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Alternate = GPIO_AF7_USART1;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  GPIO_InitStructure.Pull = GPIO_NOPULL;

  // PA9 is UART1 TX
  GPIO_InitStructure.Pin = GPIO_PIN_9;
  GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // PA10 is UART1 RX
  GPIO_InitStructure.Pin = GPIO_PIN_10;
  GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_RED_Pin|LD3_Pin|LED_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_RED_Pin LD3_Pin LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_RED_Pin|LD3_Pin|LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if(htim == &htim2) {
        tim2_tick_count++;
    }

    if((htim == &htim16)) {
        ts_interrupt = true;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart == &huart1) {
        lora.copying_bytes = false;
        lora.msg_end = false;
        lora.buf_idx = 0;
        lora.msg_len = 0;

    }else if(huart == &huart2) {
        if(lora.echo_resp){
            lora.echo_resp = false;
            lora.msg_end = true;
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) 
{
    if(huart == &huart1){
        if(lora.byte == '+'){
            lora.copying_bytes = true;
        }

        if(lora.copying_bytes && !lora.msg_end){
            lora.buf[lora.buf_idx]=lora.byte;
            lora.buf_idx++;

            HAL_UART_Receive_IT(&huart1, &lora.byte, 1);
        }

        if(lora.copying_bytes && lora.byte == '\n' && lora.prev_byte == '\r'){
            lora.msg_len = lora.buf_idx;
            // lora.msg_end = true;
            memcpy(lora_echo_buf, lora.buf, lora.msg_len);
            lora.echo_resp = true;
            HAL_UART_Transmit_IT(&huart2, lora_echo_buf, lora.msg_len);
        }

        lora.prev_byte = lora.byte;
    }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
