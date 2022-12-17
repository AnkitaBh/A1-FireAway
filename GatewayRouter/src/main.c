/**
  * @file           : main.c
  * @brief          : Network protocol stack for GW router & sleeping modes
  * @author         : Arden Diakhate-Palme
  *                   Modified by Karen Arbuzzo
  * @date           : 14.12.22
**/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h> 
#include <sys/time.h>

#include "../inc/lora.h"
#include "../inc/protocols.h"
#include "../inc/gpio.h"
#include "../inc/json_write.h"

/***** CONFIGURATION: change before uploading to each board ***/
const int NUM_NODES = 9;
const enum topology_t NETWORK_TOPOLOGY = MESH_9;
#define RESET_NODE 1 
#define LORA_DEBUG 0 
#define DBG_RECV_DATA 0 
#define TEST_RDT 0
const int GW_ROOT_ADDR = 1;
const int NODE_ADDR = GW_ROOT_ADDR;

const int MAX_STP_ITER = 1;
const int STP_TS = 4000; // in milliseconds
const int STP_CYCLE = (NUM_NODES + 1) * STP_TS;
const int MAX_PKT_SZ = 240;
const int DATA_TS = 8000; //in milliseconds
const int DC_TS = 2000; // in milliseconds
const int TEMP_THRESH = 1000000; // degrees C increase to detect fire
const int DATA_RECV_WIN = 2000; // in milliseconds
const int LORA_TOA = 1972; // Time on Air in ms
const int TIME_BUF = 800;

const uint32_t LINE_6_DELAY = 284060; //ms (the time at which the last node receives its schedule)
const uint32_t FULL_6_DELAY = 187699;
// const uint32_t MESH_9_DELAY = 339488 + 90000;
const uint32_t MESH_9_DELAY = 433049;
const uint32_t DELAY_TO_PHASE4 = MESH_9_DELAY + (LORA_TOA * 15); // in milliseconds 

const int STM_CONFIG_TIME = 4802; //ms
const int STANDBY_TIME = 60; // sec
const int TEST_SETUP_TIME = 90; //sec
// const int TEST_SETUP_TIME = 0; //sec
/***** END CONFIGURATION ***/

struct timeval start_sim_time;

int main() {
    init_UART();
    init_GPIO();

    struct timeval start_time, end_time, curr_time;
    int elapsed_time = 0;
    int err=0;
    int button_val;

    // configure initial mesh topology
    mixnet_node_config_t config;
    config.node_addr = NODE_ADDR;
    set_topology(&config, NUM_NODES, NETWORK_TOPOLOGY);

    /* STP variables */
    int initial_stp_timeout = config.node_addr * STP_TS; // initial timeout to receive updates before transmitting your STP advertisement
    int stp_timeout;
    int stp_remaining_time;
    int stp_iter = 0;
    int num_broadcast_rxd = 0;
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
    uint8_t num_tree_children = 0; 
    graph_t *net_graph;

    /* SCH variables */
    schedule_t node_sch;
    data_sch_config_t sch_config;
    init_sch(&node_sch);
    init_sch_config(&sch_config);

    /* DATA variables */
    data_config_t sensor_db;
    packet_data_t *recvd_data_pkt;
    init_sensor_db(&config, &sensor_db);
    
    // initialize transcievers
    set_red_led();
    gettimeofday(&start_time, NULL);
    if((err=init_LORA()) < 0){
        fprintf(stderr, "LORA INITIALIZATION ERROR\n");
    }
    sleep(1);
    #if RESET_NODE
    if((err=config_LORA(config.node_addr)) < 0){
        fprintf(stderr, "LORA CONFIGURATION ERROR\n");
    }
    #endif

    clear_red_led();
    gettimeofday(&end_time, NULL);
    elapsed_time = ((end_time.tv_sec * 1000 + (end_time.tv_usec / 1000)) - (start_time.tv_sec * 1000 + (start_time.tv_usec / 1000)));
    printf("[configuration took %d ms]\n", elapsed_time);

    char *raw_packet;
    mixnet_packet_t *recvd_packet;
    int recv_port = 0;
    err_code_t err_code;


    #if TEST_BTN
    while(1){
        button_val = 1;
        while(button_val) {
            button_val = get_button_val();
        }
        printf("button pressed!\n");
        while(!button_val) {
            button_val = get_button_val();
        }
        printf("button released!\n\n");
    }
    #endif
    
    button_val = 1;
    while(button_val) {
        button_val = get_button_val();
    }
    printf("button pressed!\n");
    while(!button_val) {
        button_val = get_button_val();
    }
    printf("button released!\n\n");

    // Cannot reset the LORA after configuring it!
    // char recv_config_buf[40];
    // read_resp(recv_config_buf, " +READY\r\n");
    // printf("received: %s", recv_config_buf);

    #if TEST_RDT
    while(1){        
            set_green_led();
            usleep(STM_CONFIG_TIME * 1000);
            clear_green_led();

            printf("[%d] Sending 3 packets...\n", config.node_addr);
            for(int i=0; i<3; i++){
                send_lsa_req(&config, 2);
            }

            usleep(1000000);

            printf("[%d] Receiving 3 packets..\n", config.node_addr);
            for(int i=0; i<3; i++) {
                raw_packet = rdt_recv_pkt(&config);
                assert(raw_packet != NULL);
                recvd_packet = (mixnet_packet_t*)raw_packet;
                printf("Application received packet [SEQ %u] from %u\n", recvd_packet->seqnum, recvd_packet->src_address);
            }
    }
    #endif

    for(int i=0; i<TEST_SETUP_TIME; i++)
        usleep(1000000);

    gettimeofday(&start_sim_time, NULL);


    #if LORA_DEBUG
    char cmds[5][20] = {
        "AT+SEND=0,5,seq"
    };

    if((err=init_LORA()) < 0){
        fprintf(stderr, "LORA INITIALIZATION ERROR\n");
    }

    char cmd_str[40];
    int iter=0;

    int timeout = 500;

    while(1){
        // send_ack_pkt(&config, 6, 10);
        // sleep(1);

        // raw_packet = lora_recv_data(timeout, &err_code); // +1 second

        // // if we actually received a packet 
        // if(raw_packet != NULL) {
        //     recvd_packet = (mixnet_packet_t*)raw_packet;
        //     printf("[%d] received packet type %u from %u\n", config.node_addr, recvd_packet->type, recvd_packet->src_address);
        // }else{
        //     printf("packet timeout after %d ms\n", timeout);
        // }

        send_lsa_req(&config, 6);
    }

    #endif


    #if DBG_RECV_DATA
    broadcast_stp(&config, &stp_route_db);
    while(1){
        raw_packet = lora_recv_data(500); // +1 second

        // if we actually received a packet 
        if(raw_packet != NULL) {
            recvd_packet = (mixnet_packet_t*)raw_packet;
            printf("[%d] received packet type %u from %u\n", config.node_addr, recvd_packet->type, recvd_packet->src_address);
        }
        printf("timeout\n");
        sleep(1);
    }
    #endif /* DBG_RECV_DATA */


    bool goto_next_proto_iter = false;
    int protcol_iteration = 1;
    while(1) {
        printf("Protocol iteration %d\n", protcol_iteration);
        
        set_green_led();
        usleep(STM_CONFIG_TIME * 1000);
        clear_green_led();

        // TODO: reset all data structures
        stp_iter = 0;
        stp_route_db.root_address = config.node_addr;
        stp_route_db.path_length = 0;
        stp_route_db.next_hop_address = config.node_addr;
        activate_all_ports(stp_route_db.stp_ports, config.num_neighbors);

        net_graph = graph_init();
        (void)graph_add_neighbors(net_graph, config.node_addr, config.neighbor_addrs, config.num_neighbors);

        reset_sch_config(&sch_config);
        reset_sensor_db(&config, &sensor_db);
        reset_sch(&node_sch);

        /* TDM STP PROTOCOL */
        // TODO: invalidate unresponsive nodes in 2nd round of STP even if they replied in 1st
        printf("============= Phase 1: STP =============\n");
        set_red_led(); // set the indicate the root at onset (shine red LED)
        while(stp_iter < 2 * MAX_STP_ITER) {
            if(stp_iter % 2 == 0) {
                stp_txen = true;
                stp_timeout = initial_stp_timeout;
            }else{
                stp_txen = false;
                stp_timeout = STP_CYCLE - initial_stp_timeout;
            }

            num_broadcast_rxd = 0;
            elapsed_time = 0;
            gettimeofday(&start_time, NULL);
            do {
                if(num_broadcast_rxd == 0){
                    stp_remaining_time = stp_timeout;

                }else{
                    #if DBG_STP
                    printf("[iteration: %d, timeout %d]\n", (stp_iter % 2) + 1, stp_remaining_time);
                    #endif

                    gettimeofday(&end_time, NULL);
                    elapsed_time = ((end_time.tv_sec * 1000 + (end_time.tv_usec / 1000)) - 
                                    (start_time.tv_sec * 1000 + (start_time.tv_usec / 1000)));
                    stp_remaining_time = (stp_timeout - elapsed_time + 40);
                }

                num_broadcast_rxd++;

                raw_packet = lora_recv_data(stp_remaining_time, &err_code);

                #if DBG_STP
                printf("elapsed time: %d ms\n", elapsed_time);
                printf("remaining time: %d ms\n", stp_remaining_time);
                #endif

                // if we actually received a packet 
                if(raw_packet != NULL) {
                    recvd_packet = (mixnet_packet_t*)raw_packet;

                    #if DBG_STP
                    printf("[%d] received packet type %u from %u\n", config.node_addr, recvd_packet->type, recvd_packet->src_address);
                    #endif
                    
                    // was the packet from a neighbor?
                    recv_port = get_port(config.neighbor_addrs, config.num_neighbors, recvd_packet->src_address);
                    if(recv_port != -1 && (recvd_packet->type == PACKET_TYPE_STP)) {
                        recvd_stp_pkt = (mixnet_packet_stp_t*)recvd_packet->payload;

                        printf("[%d] received STP packet: (%d, %d, %d)\n", 
                        config.node_addr, recvd_stp_pkt->root_address, recvd_stp_pkt->path_length, recvd_stp_pkt->node_address);

                        recvd_from_ports[recv_port] = 1;

                        update_stp_db(&config, (uint8_t)recv_port, &stp_route_db, recvd_stp_pkt);
                    }

                    free(recvd_packet);
                }
            }while(stp_remaining_time >= STP_TS);

            if(stp_txen) {
                printf("[%d] broadcasting STP update...\n", config.node_addr);
                broadcast_stp(&config, &stp_route_db);
            }

            stp_iter++;
        }

        // printf("STP remaining time: %d ms\n", stp_remaining_time);
        if(stp_remaining_time > 0){
            usleep((stp_remaining_time * 1000) + 50000);
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


        clear_red_led();
        sleep(3);

        //TODO: must be able to handle node failures after STP: if after N timeouts on rdt_send(), 
        //      if we do not get a response, the node is deemed offline
        /* LSA PROTOCOL */
        printf("============= Phase 2: LSA =============\n");
        num_tree_children = get_child_node_lsa(&config, &stp_route_db, net_graph);
        (void)num_tree_children;

        print_graph(net_graph);
        sleep(3);

        /* RX/TX SCHEDULE */
        printf("============= Phase 3: SCH =============\n");
        populate_schedule(net_graph, &node_sch);
        print_schedule(&node_sch);

        // TODO schedule generated but not used by nodes 
        query_sch(&config, &stp_route_db, &node_sch, DELAY_TO_PHASE4);
        
        sleep(3);

        /* DATA Phase */
        printf("============= Phase 4: DATA ============\n");
        printf("Listening for data from %u neighbors:\n", num_tree_children);

        // TODO inactive links for offline nodes
        for(int i=0; i<num_tree_children; i++){
            raw_packet = rdt_recv_pkt(&config);
            if(raw_packet != NULL){
                recvd_packet = (mixnet_packet_t*)raw_packet;

                printf("[%d] received packet type %u from %u\n", config.node_addr, recvd_packet->type, recvd_packet->src_address);

                recv_port = get_port(config.neighbor_addrs, config.num_neighbors, recvd_packet->src_address);
                if(recv_port != -1 && (recvd_packet->type == PACKET_TYPE_DATA)) {
                    recvd_data_pkt = (packet_data_t*)recvd_packet->payload;
                    populate_sensor_data(&sensor_db, recvd_data_pkt);
                }
                free(recvd_packet);
            }
        }

        print_sensor_db(&sensor_db);
        json_write(&config, net_graph, &sensor_db);

        usleep((DATA_TS + 500) * 1000); 
        printf("nodes move to sleep\n");

        for(int i=0; i<STANDBY_TIME; i++)
            usleep(1000000);

        free_graph(net_graph);

        protcol_iteration++;
    }

    close_GPIO();
    return 0;
}