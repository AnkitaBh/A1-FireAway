/**
  * @file           : protocols.c
  * @brief          : Networking protocol stack functions (Phases: STP, LSA, SCH, DATA) and networking helper functions.
  * @author         : Arden Diakhate-Palme
  * @date           : 14.12.22
**/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stm32l4xx_hal.h"

#include "packet.h"
#include "protocols.h"
#include "lora.h"
#include "uart.h"
#include "graph.h"
#include "prints.h"

extern int NUM_NODES;
extern int MAX_PKT_SZ;

extern UART_HandleTypeDef huart1; // Lora UART
extern UART_HandleTypeDef huart2; // Debug UART

extern uint32_t tim2_tick_count; 
extern uint64_t LSA_TIMEOUT; 
extern uint32_t ACK_TIMEOUT; 
extern int LORA_TOA;
extern int TIME_BUF;

extern int PRINT_TIME;
extern int PKT_TX_DELAY; 

static const int MAX_BACKOFF = 240000;
static const int MAX_RETX_ATTEMPTS = 3; // this will depend on the delay sent out in the SCH phase from the GW

/* STP Functions */
void update_stp_db(mixnet_node_config_t *config,  uint8_t recv_port, stp_route_t *stp_route_db, 
                   mixnet_packet_stp_t *recvd_stp_packet) {


    // Receive STP message from smaller ID root node
    // Make him root and increment path length
    if(recvd_stp_packet->root_address < stp_route_db->root_address) {

        stp_route_db->root_address = recvd_stp_packet->root_address;
        stp_route_db->path_length = recvd_stp_packet->path_length + 1;
        stp_route_db->next_hop_address = recvd_stp_packet->node_address;

        stp_route_db->stp_parent_addr = recvd_stp_packet->node_address;
        stp_route_db->stp_parent_path_length = recvd_stp_packet->path_length;                        

        stp_route_db->stp_ports[recv_port] = 1; //Open parent port

        /* We're other nodes every time it's our turn to transmit, if we heard from others or not */

    }else if(recvd_stp_packet->root_address == stp_route_db->root_address) {
        // Recieve from same ID, shorter path root node
        // Follow his path instead, increment path length
        if(recvd_stp_packet->path_length + 1 < stp_route_db->path_length) {
            stp_route_db->next_hop_address = recvd_stp_packet->node_address;
            stp_route_db->path_length = recvd_stp_packet->path_length + 1;
                                        
            stp_route_db->stp_parent_addr = recvd_stp_packet->node_address;
            stp_route_db->stp_parent_path_length = recvd_stp_packet->path_length;   
            
            stp_route_db->stp_ports[recv_port] = 1; //Open parent port
        
        //Tie for ID and path length
        } else if (stp_route_db->stp_parent_addr != -1 && recvd_stp_packet->path_length == stp_route_db->stp_parent_path_length){
            
            if (recvd_stp_packet->node_address < stp_route_db->stp_parent_addr){
                // Close port for to-be-removed parent. Open port for new parent
                port_to_neighbor(config, stp_route_db->stp_parent_addr, stp_route_db->stp_ports, BLOCK_PORT);
                port_to_neighbor(config, recvd_stp_packet->node_address, stp_route_db->stp_ports, OPEN_PORT);

                // choose lesser indexed node to route through
                stp_route_db->next_hop_address = recvd_stp_packet->node_address;
                stp_route_db->stp_parent_addr = recvd_stp_packet->node_address;         
                stp_route_db->stp_parent_path_length = recvd_stp_packet->path_length;   
                

            }else if (recvd_stp_packet->node_address > stp_route_db->stp_parent_addr){
                // Close port for failed parent candidate
                port_to_neighbor(config, recvd_stp_packet->node_address, stp_route_db->stp_ports, BLOCK_PORT);
            }
        }

        // If path advertised is equal to path length, node must be peer, not {child, parent} 
        if (recvd_stp_packet->path_length == stp_route_db->path_length){
                stp_route_db->stp_ports[recv_port] = 0;
        }
    } else {
        port_to_neighbor(config, recvd_stp_packet->node_address, stp_route_db->stp_ports, OPEN_PORT); //Open child port
    }

    #if DBG_STP
    char printf_buf[40];
    HAL_printf( "STP DB: [%u, %u, %u]; parent: %u (len %u)\n", 
        stp_route_db->root_address, stp_route_db->path_length, stp_route_db->next_hop_address,
        stp_route_db->stp_parent_addr, stp_route_db->stp_parent_path_length);
    
    #endif
}

void broadcast_stp(mixnet_node_config_t *config, stp_route_t *stp_route_db) {
    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sizeof(mixnet_packet_stp_t));
    mixnet_packet_stp_t stp_payload;

    pkt->src_address = config->node_addr;
    pkt->dst_address = 0;
    pkt->type = PACKET_TYPE_STP;
    pkt->payload_size = sizeof(mixnet_packet_stp_t);

    stp_payload.root_address = stp_route_db->root_address; 
    stp_payload.path_length = stp_route_db->path_length;
    stp_payload.node_address = config->node_addr;
    memcpy(pkt->payload, &stp_payload, sizeof(mixnet_packet_stp_t));

    LORA_send_packet(&huart1, &huart2, pkt);
    free(pkt);
    
    HAL_printf("[%d] broadcasting STP update: (%u, %u, %u)\n", 
                        config->node_addr, stp_route_db->root_address, stp_route_db->path_length, config->node_addr);
}


/* LSA Functions */
int send_lsa_req(mixnet_node_config_t *config, mixnet_address dst_addr) {
    int ret_val = 0;
    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sizeof(mixnet_packet_lsa_t));
    mixnet_packet_lsa_t lsa_payload;

    pkt->src_address = config->node_addr;
    pkt->dst_address = dst_addr;
    pkt->type = PACKET_TYPE_LSA;
    pkt->payload_size = sizeof(mixnet_packet_lsa_t);

    lsa_payload.flags = LSA_REQ;
    lsa_payload.node_address = 0;
    lsa_payload.neighbor_count = 0;
    memcpy(pkt->payload, &lsa_payload, sizeof(mixnet_packet_lsa_t));

    ret_val = rdt_send_pkt(&huart1, &huart2, config, pkt);
    return ret_val;
}

void send_lsa_inv(mixnet_node_config_t *config, mixnet_address dst_addr) {
    int ret_val = 0;
    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sizeof(mixnet_packet_lsa_t));
    mixnet_packet_lsa_t lsa_payload;

    pkt->src_address = config->node_addr;
    pkt->dst_address = dst_addr;
    pkt->type = PACKET_TYPE_LSA;
    pkt->payload_size = sizeof(mixnet_packet_lsa_t);

    lsa_payload.flags = LSA_INV;
    lsa_payload.node_address = 0;
    lsa_payload.neighbor_count = 0;
    memcpy(pkt->payload, &lsa_payload, sizeof(mixnet_packet_lsa_t));

    ret_val = rdt_send_pkt(&huart1, &huart2, config, pkt);
    //return ret_val;
}

int get_child_node_lsa(mixnet_node_config_t *config, stp_route_t *stp_route_db, graph_t *net_graph) {
    char *raw_packet;
    int recv_port;
    mixnet_packet_t *recvd_packet;
    mixnet_packet_lsa_t *recvd_lsa_pkt;

    int num_lsa_req_fwd = 0;
    int err=0;

    for(int i=0; i<config->num_neighbors; i++) {
        if(stp_route_db->stp_ports[i]) {

            HAL_printf("[%d] forwarding LSA REQ to %u\n", config->node_addr, config->neighbor_addrs[i]);

            if((err=send_lsa_req(config, config->neighbor_addrs[i])) < 0){
                remove_tree_neighbor(config, net_graph, config->neighbor_addrs[i]);
                continue;
            }

            #if DBG_LSA
            HAL_printf("[%d] waiting for LSA RESP from %u\n", config->node_addr, config->neighbor_addrs[i]);
            #endif

            while(1){
                raw_packet = rdt_recv_pkt(&huart1, &huart2, config);

                // parse LSA response from child node
                if(raw_packet != NULL){
                    recvd_packet = (mixnet_packet_t*)raw_packet;

                    // only parse LSA packets from tree neighbors           
                    recv_port = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
                    if(recv_port != -1 && (recvd_packet->type == PACKET_TYPE_LSA)) {
                        recvd_lsa_pkt = (mixnet_packet_lsa_t*)recvd_packet->payload;
                            #if DBG_LSA
                            HAL_printf("[%d] received LSA packet from node %d\n", config->node_addr, recvd_packet->src_address); 
                            #endif

                            if(recvd_lsa_pkt->flags & LSA_RESP){
                                add_to_graph(net_graph, recvd_lsa_pkt);
                            }else if(recvd_lsa_pkt->flags & LSA_INV){
                                stp_route_db->stp_ports[i] = 0;
                                #if DBG_LSA
                                HAL_printf("[%d] received LSA packet from node %d\n", config->node_addr, recvd_packet->src_address); 
                                #endif
                                // remove_tree_neighbor(config, net_graph, config->neighbor_addrs[i]);
                                // print_graph(net_graph);
                            }else{
                                #if DBG_LSA
                                HAL_printf("[%u] ERROR: expected LSA_RESP or LSA_INV, received %u\n", config->node_addr, recvd_lsa_pkt->flags);
                                #endif
                            }

                            if(recvd_lsa_pkt->flags == (LSA_RESP | LSA_RESP_END) || recvd_lsa_pkt->flags == (LSA_INV)) 
                                break;
                    }
                    free(recvd_packet);
                }
            }

            HAL_printf("[%d] complete LSA response received from %u\n", config->node_addr, config->neighbor_addrs[i]);


            num_lsa_req_fwd++;
        }
    }

    return num_lsa_req_fwd;
}

int send_lsa_resp_pkt(mixnet_node_config_t *config, mixnet_address parent_addr,
                       mixnet_address *neighbor_list, mixnet_address lsa_node_addr, uint16_t num_nbors, uint8_t flags) {
    int ret_val = 0;
    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sizeof(mixnet_packet_lsa_t) + (sizeof(mixnet_address) * num_nbors));
    mixnet_packet_lsa_t lsa_payload;

    pkt->src_address = config->node_addr;
    pkt->dst_address = parent_addr;
    pkt->type = PACKET_TYPE_LSA;
    pkt->payload_size = sizeof(mixnet_packet_lsa_t) + (sizeof(mixnet_address) * num_nbors);

    lsa_payload.node_address = lsa_node_addr;
    lsa_payload.neighbor_count = num_nbors;
    lsa_payload.flags = flags;
    memcpy(pkt->payload, &lsa_payload, sizeof(mixnet_packet_lsa_t));

    mixnet_address* neighbors_start = (mixnet_address*)((mixnet_packet_lsa_t*)pkt->payload + 1);
    memcpy(neighbors_start, neighbor_list, sizeof(mixnet_address) * num_nbors);

    ret_val = rdt_send_pkt(&huart1, &huart2, config, pkt);
    return ret_val;
}

int send_lsa_resp(mixnet_node_config_t *config, stp_route_t *stp_route_db, graph_t *net_graph) {
    int err, ret_val = 0;
    mixnet_address neighbor_list[20];
    int nbor_idx = 0;
    adj_vert_t *vertex = NULL;
    adj_node_t *node = NULL;
    uint8_t lsa_flags = LSA_RESP;
    int node_port = 0;

    print_graph(net_graph);

    vertex = net_graph->head;
    while(vertex != NULL) {
        if(vertex->addr == config->node_addr) {
            nbor_idx = 0;
            node = vertex->adj_list;
            while(node != NULL){
                node_port = get_port(config->neighbor_addrs, config->num_neighbors, node->addr);
                if(node_port != -1 && stp_route_db->stp_ports[node_port]){
                    neighbor_list[nbor_idx++] = node->addr;
                }
                node = node->next;
            }
        }else{
            nbor_idx = 0;
            node = vertex->adj_list;
            while(node != NULL){
                neighbor_list[nbor_idx++] = node->addr;
                node = node->next;
            }
        }

        if(vertex->next_vert == NULL){
            lsa_flags |= LSA_RESP_END;
        }
        
        #if DBG_LSA
        HAL_printf("[%d] sending LSA response (%u) to %u\n", config->node_addr, lsa_flags, stp_route_db->stp_parent_addr); 
        #endif
        if((err=send_lsa_resp_pkt(config, stp_route_db->stp_parent_addr, neighbor_list, vertex->addr, nbor_idx, lsa_flags)) < 0){
            #if DG_LSA
            HAL_printf("[%d] Max transmission attempts reached!!\n", config->node_addr); 
            #endif
            ret_val = -1;
        }

        // wait_ms(500); // processing delay of LSA RESP upstream

        vertex = vertex->next_vert;
    }
    return ret_val;
}

void add_to_graph(graph_t *net_graph, mixnet_packet_lsa_t *recvd_lsa_pkt) {
    mixnet_address neighbor_list[NUM_NODES];
    mixnet_address *nbor = (mixnet_address*)((char*)recvd_lsa_pkt + sizeof(mixnet_packet_lsa_t));
    for(int i=0; i<recvd_lsa_pkt->neighbor_count; i++){
        neighbor_list[i] = *nbor;
        nbor = (mixnet_address*)((char*)nbor + sizeof(mixnet_address));
    }
    (void)graph_add_neighbors(net_graph, recvd_lsa_pkt->node_address, 
                              neighbor_list, recvd_lsa_pkt->neighbor_count);
}

/* SCH RX/TX Functions */
void send_sch_ack(mixnet_node_config_t *config, mixnet_address dst_addr) {
    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sizeof(packet_sch_t));
    packet_sch_t sch_payload;

    pkt->src_address = config->node_addr;
    pkt->dst_address = dst_addr;
    pkt->type = PACKET_TYPE_SCH;
    pkt->payload_size = sizeof(packet_sch_t);

    sch_payload.total_ts = 0;
    sch_payload.rxtx_list_len = 0;
    sch_payload.tx_ts = 0;
    sch_payload.flags = SCH_ACK;
    memcpy(pkt->payload, &sch_payload, sizeof(packet_sch_t));

    rdt_send_pkt(&huart1, &huart2, config, pkt);
}

void query_sch(mixnet_node_config_t *config, stp_route_t *stp_route_db, schedule_t *node_sch, uint32_t delay_to_phase4) {
    char printf_buf[40];
    char *raw_packet;
    mixnet_packet_t *recvd_packet;
    int recv_port = 0;
    packet_sch_t *recvd_sch_pkt;

    for(int i=0; i<config->num_neighbors; i++) {
        if(stp_route_db->stp_ports[i]) {
            HAL_printf("[%d] sending schedule to %u\n", config->node_addr, config->neighbor_addrs[i]);

            send_sch(config, node_sch, delay_to_phase4, config->neighbor_addrs[i]);

            HAL_printf("[%d] waiting for SCH ACK from %u\n", config->node_addr, config->neighbor_addrs[i]);

            while(1){ 
                raw_packet = rdt_recv_pkt(&huart1, &huart2, config);

                if(raw_packet != NULL){
                    recvd_packet = (mixnet_packet_t*)raw_packet;

                    recv_port = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
                    if(recv_port != -1 && (recvd_packet->type == PACKET_TYPE_SCH)) {
                        recvd_sch_pkt = (packet_sch_t*)recvd_packet->payload;

                        if(recvd_sch_pkt->flags == SCH_ACK && (recvd_packet->src_address == config->neighbor_addrs[i])) {
                            HAL_printf("[%d] received SCH ACK from node %u\n", config->node_addr, recvd_packet->src_address);
                            break;
                        }
                    }
                    free(recvd_packet);
                }
            }

            HAL_printf(printf_buf, "[%d] Node %u has its sub-schedule\n", config->node_addr, config->neighbor_addrs[i]);
        }
    }
}

void send_sch_pkt(mixnet_node_config_t *config, uint32_t delay_to_phase4, timeslot_t tx_slot, int rxtx_list_len, 
                mixnet_address *tx_list, mixnet_address *rx_list, mixnet_address *recv_order_list, mixnet_address dst_node_addr) {

    uint8_t sch_payload_size = sizeof(packet_sch_t) 
                            + (sizeof(mixnet_address) * rxtx_list_len) 
                            + (sizeof(mixnet_address) * rxtx_list_len) 
                            + (sizeof(mixnet_address) * (NUM_NODES-1));

    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sch_payload_size);
    packet_sch_t sch_payload;

    pkt->src_address = config->node_addr;
    pkt->dst_address = dst_node_addr;
    pkt->type = PACKET_TYPE_SCH;
    pkt->payload_size = sch_payload_size;

    sch_payload.total_ts = NUM_NODES - 1;
    sch_payload.tx_ts = tx_slot; 
    sch_payload.rxtx_list_len = rxtx_list_len;
    sch_payload.delay_to_phase4 = delay_to_phase4;
    memcpy(pkt->payload, &sch_payload, sizeof(packet_sch_t));

    // copy RX slots
    char* rx_list_begin = (char*)((char*)pkt->payload + sizeof(packet_sch_t));
    memcpy(rx_list_begin, rx_list, (sizeof(mixnet_address) * rxtx_list_len));

    // copy TX slots
    char* tx_list_begin = (char*)((char*)pkt->payload + sizeof(packet_sch_t)
                            + (sizeof(mixnet_address) * rxtx_list_len));
    memcpy(tx_list_begin, tx_list, sizeof(mixnet_address) * rxtx_list_len);

    // copy entire recv order
    char* recv_order_begin = (char*)((char*)pkt->payload + sizeof(packet_sch_t)
                            + (sizeof(mixnet_address) * rxtx_list_len) 
                            + (sizeof(mixnet_address) * rxtx_list_len));
    memcpy(recv_order_begin, recv_order_list, sizeof(mixnet_address) * (NUM_NODES - 1));

    rdt_send_pkt(&huart1, &huart2, config, pkt);
}

void send_sch(mixnet_node_config_t *config, schedule_t *global_sch, uint32_t delay_to_phase4, mixnet_address dst_node_addr) {
    timeslot_t tx_slot;
    mixnet_address *rx_list = malloc(sizeof(mixnet_address) * (NUM_NODES - 1));
    mixnet_address *tx_list = malloc(sizeof(mixnet_address) * (NUM_NODES - 1));
    int rxtx_idx=0;

    // until the node transmit, capture all slots
    for(timeslot_t ts=0; ts<NUM_NODES-1; ts++) {
        tx_list[rxtx_idx] = global_sch->tx_list[ts];
        rx_list[rxtx_idx] = global_sch->rx_list[ts];
        rxtx_idx++;

        if(global_sch->tx_list[ts] == dst_node_addr) {
            tx_slot = ts;
            break;
        }
    }

    send_sch_pkt(config, delay_to_phase4, tx_slot, 
                rxtx_idx, tx_list, rx_list, global_sch->sch_recv_order, 
                dst_node_addr);
    free(rx_list);
    free(tx_list);
}

// requires the sch_config has RX_TS array allocated 
void populate_sch_config(mixnet_node_config_t * config, schedule_t *sch, data_sch_config_t *sch_config) {
    int rx_slots_idx = 0;
    int num_rx_slots = 0;
    timeslot_t tx_slot;
    timeslot_t recv_sch_timeslot;
    
    for(timeslot_t ts=0; ts<sch->total_ts; ts++) {
        if(sch->tx_list[ts] == config->node_addr) {
            tx_slot = ts;
        }

        if(sch->rx_list[ts] == config->node_addr) {
            sch_config->rx_ts[rx_slots_idx++] = ts;
        }

        if(sch->sch_recv_order[ts] == config->node_addr) {
            recv_sch_timeslot = ts;
        }
    }
    num_rx_slots = rx_slots_idx;

    sch_config->num_rx_ts = num_rx_slots;
    sch_config->tx_ts = tx_slot;
    sch_config->total_ts = sch->total_ts;
    (void)recv_sch_timeslot;
}


void populate_local_sch(schedule_t *local_sch, packet_sch_t *recvd_sch_pkt) {
    // the entire schedule up to me is what I received
    char *recv_timeslot = (char*)((char*)recvd_sch_pkt + sizeof(packet_sch_t));
    memcpy(local_sch->rx_list, recv_timeslot, (recvd_sch_pkt->rxtx_list_len  * sizeof(timeslot_t)));

    char *tx_timeslot = (char*)((char*)recvd_sch_pkt + sizeof(packet_sch_t) + (recvd_sch_pkt->rxtx_list_len * sizeof(timeslot_t)));
    memcpy(local_sch->tx_list, tx_timeslot, (recvd_sch_pkt->rxtx_list_len * sizeof(timeslot_t)));

    // the entire receive order is sent
    char *recv_order = (char*)((char*)recvd_sch_pkt + sizeof(packet_sch_t) 
                                    + (recvd_sch_pkt->rxtx_list_len * sizeof(timeslot_t))
                                    + (recvd_sch_pkt->rxtx_list_len * sizeof(timeslot_t)));
    memcpy(local_sch->sch_recv_order, recv_order, (recvd_sch_pkt->total_ts * sizeof(timeslot_t)));

    local_sch->total_ts = recvd_sch_pkt->total_ts;
}

void init_sch_config(data_sch_config_t *sch_config) {
    sch_config->rx_ts = malloc(sizeof(timeslot_t) * NUM_NODES);
    sch_config->tx_ts = 0;
    sch_config->num_ts_sleep = 0;
    sch_config->num_rx_ts = 0;
}

void print_sch_config(data_sch_config_t *sch_config) {
    HAL_printf("TX TS: %u\n", sch_config->tx_ts);

    HAL_printf("RX TS: [ ");
    for(int i=0; i<sch_config->num_rx_ts; i++){
        HAL_printf("%u ", sch_config->rx_ts[i]);
    }
    HAL_printf("]\n");

    HAL_printf("total TS: %u\n", sch_config->total_ts);

    HAL_printf("sleep for: %u TS\n", sch_config->num_ts_sleep);

    HAL_printf("current time: %u\n", tim2_tick_count);
}

uint16_t get_responsive_child_nodes(mixnet_node_config_t *config, stp_route_t *stp_route_db, graph_t *net_graph, 
                                    mixnet_address *child_nodes, mixnet_address parent_node, mixnet_address node) {
    adj_vert_t *vert = find_vertex(net_graph, node);
    adj_node_t *nbor = vert->adj_list;

    int port;
    uint16_t idx=0;
    while(nbor != NULL){
        port = get_port(config->neighbor_addrs, config->num_neighbors, nbor->addr);
        if(nbor->addr != parent_node &&
           port != -1 &&
           stp_route_db->stp_ports[port]) {

            child_nodes[idx++] = nbor->addr;
        }
        nbor = nbor->next;
    }
    return idx;
}

/* DATA phase functions */
void send_sensor_data(mixnet_node_config_t *config, data_config_t *sensor_db, mixnet_address dst_node_addr) {

    uint8_t sch_payload_size = sizeof(packet_data_t) 
                            + (sizeof(mixnet_address) * sensor_db->num_nodes_on_fire) 
                            + (sizeof(mixnet_address) * sensor_db->num_resp_nodes);

    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sch_payload_size);
    packet_data_t data_payload;

    pkt->src_address = config->node_addr;
    pkt->dst_address = dst_node_addr;
    pkt->type = PACKET_TYPE_DATA;
    pkt->payload_size = sch_payload_size;

    data_payload.num_nodes_on_fire = sensor_db->num_nodes_on_fire;
    data_payload.num_resp_nodes = sensor_db->num_resp_nodes;
    memcpy(pkt->payload, &data_payload, sizeof(packet_data_t));

    // copy nodes on fire
    char *node_on_fire = (char*)((char*)pkt->payload + sizeof(packet_data_t));
    memcpy(node_on_fire, sensor_db->nodes_on_fire, (sizeof(mixnet_address) * sensor_db->num_nodes_on_fire));

    // copy responsive nodes
    char *resp_nodes = (char*)((char*)pkt->payload + sizeof(packet_data_t)
                                + (sizeof(mixnet_address) * sensor_db->num_nodes_on_fire));
    memcpy(resp_nodes, sensor_db->resp_nodes, (sizeof(mixnet_address) * sensor_db->num_resp_nodes));

    rdt_send_pkt(&huart1, &huart2, config, pkt);
}

void populate_sensor_data(data_config_t *sensor_db, packet_data_t *recvd_data_pkt) {
    char *nodes_on_fire_db_end = (char*)((char*)sensor_db->nodes_on_fire + (sensor_db->num_nodes_on_fire * sizeof(mixnet_address)));
    char *nodes_on_fire = (char*)((char*)recvd_data_pkt + sizeof(packet_data_t));
    memcpy(nodes_on_fire_db_end, nodes_on_fire, (recvd_data_pkt->num_nodes_on_fire  * sizeof(mixnet_address)));

    char *resp_nodes_db_end = (char*)((char*)sensor_db->resp_nodes + (sensor_db->num_resp_nodes * sizeof(mixnet_address)));
    char *resp_nodes = (char*)((char*)recvd_data_pkt + sizeof(packet_data_t) 
                                + (recvd_data_pkt->num_nodes_on_fire  * sizeof(mixnet_address)));
    memcpy(resp_nodes_db_end, resp_nodes, (recvd_data_pkt->num_resp_nodes * sizeof(mixnet_address)));

    sensor_db->num_nodes_on_fire = sensor_db->num_nodes_on_fire + recvd_data_pkt->num_nodes_on_fire;
    sensor_db->num_resp_nodes = sensor_db->num_resp_nodes + recvd_data_pkt->num_resp_nodes;
}

void print_sensor_db(data_config_t *sensor_db) {
    char printf_buf[40];
    HAL_printf("Nodes on Fire: [");
    
    for(int i=0; i<sensor_db->num_nodes_on_fire; i++){
        HAL_printf("%u ", sensor_db->nodes_on_fire[i]);
        
    }
    HAL_printf("]\n");
    

    HAL_printf("Responsive Nodes: [");
    
    for(int i=0; i<sensor_db->num_resp_nodes; i++){
        HAL_printf("%u ", sensor_db->resp_nodes[i]);
        
    }
    HAL_printf("]\n");
    
}

void init_sensor_db(mixnet_node_config_t *config, data_config_t *sensor_db) {
    sensor_db->num_nodes_on_fire = 0;
    sensor_db->nodes_on_fire = malloc(sizeof(mixnet_address) * (NUM_NODES - 1));

    sensor_db->num_resp_nodes = 1;
    sensor_db->resp_nodes = malloc(sizeof(mixnet_address) * (NUM_NODES - 1));
    sensor_db->resp_nodes[0] = config->node_addr;
}


/* Network Helper Functions */
void set_topology_mesh_7(mixnet_node_config_t *config) {
    switch (config->node_addr) {
        case 1: {
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 2;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 4;
        } break;

        case 2: {
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 1;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 5;
        } break;

        case 3: {
            config->num_neighbors = 6;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 6);
            config->neighbor_addrs[0] = 1;
            config->neighbor_addrs[1] = 2;
            config->neighbor_addrs[2] = 4;
            config->neighbor_addrs[3] = 5;
            config->neighbor_addrs[4] = 6;
            config->neighbor_addrs[5] = 7;
        } break;

        case 4: {
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 1;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 7;
        } break;

        case 5: {
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 2;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 6;
        } break;

        case 6: {
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 3;
            config->neighbor_addrs[1] = 5;
            config->neighbor_addrs[2] = 7;
        } break;

        case 7: {
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 3;
            config->neighbor_addrs[1] = 4;
            config->neighbor_addrs[2] = 6;
        } break;

        default:
            printf("INVALID Node Address\n");
            exit(1);
        break;
    }
}

void set_topology_mesh_9(mixnet_node_config_t *config) {
    switch(config->node_addr){
        case 1:{
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 2;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 4;
        } break;

        case 2:{
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 1;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 5;
        } break;

        case 3:{
            config->num_neighbors = 6;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 6);
            config->neighbor_addrs[0] = 1;
            config->neighbor_addrs[1] = 2;
            config->neighbor_addrs[2] = 4;
            config->neighbor_addrs[3] = 5;
            config->neighbor_addrs[4] = 6;
            config->neighbor_addrs[5] = 7;
        } break;

        case 4:{
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 1;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 7;
        } break;

        case 5:{
            config->num_neighbors = 4;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 4);
            config->neighbor_addrs[0] = 2;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 6;
            config->neighbor_addrs[3] = 8;
        } break;

        case 6:{
            config->num_neighbors = 5;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 5);
            config->neighbor_addrs[0] = 5;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 7;
            config->neighbor_addrs[3] = 8;
            config->neighbor_addrs[4] = 9;
        } break;

        case 7:{
            config->num_neighbors = 4;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 4);
            config->neighbor_addrs[0] = 4;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 6;
            config->neighbor_addrs[3] = 9;
        } break;

        case 8:{
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 5;
            config->neighbor_addrs[1] = 6;
            config->neighbor_addrs[2] = 9;
        } break;

        case 9:{
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 8;
            config->neighbor_addrs[1] = 6;
            config->neighbor_addrs[2] = 7;
        } break;

        default:
            printf("INVALID Node Address\n");
            exit(1);
        break;
    }
}

void set_topology_tree_4(mixnet_node_config_t *config) {
    switch(config->node_addr){
        case 1:{
            config->num_neighbors = 1;
            config->neighbor_addrs = malloc(sizeof(mixnet_address));
            config->neighbor_addrs[0] = 2;
        } break;

        case 2: {
            config->num_neighbors = 3;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * 3);
            config->neighbor_addrs[0] = 1;
            config->neighbor_addrs[1] = 3;
            config->neighbor_addrs[2] = 4;
        } break;

        case 3: {
            config->num_neighbors = 1;
            config->neighbor_addrs = malloc(sizeof(mixnet_address));
            config->neighbor_addrs[0] = 2;
        } break;

        case 4: {
            config->num_neighbors = 1;
            config->neighbor_addrs = malloc(sizeof(mixnet_address));
            config->neighbor_addrs[0] = 2;
        } break;

        default:
            UART2_tx("INVALID Node Address\n");
            exit(1);
        break;
    }
}

// requires node_address be set in the configuration
void set_topology(mixnet_node_config_t *config, uint16_t num_nodes, enum topology_t topology) {
    switch(topology) {
        case LINE: {
            if(config->node_addr == 1) {
                config->num_neighbors = 1;
                config->neighbor_addrs = malloc(sizeof(mixnet_address));
                config->neighbor_addrs[0] = 2;

            }else if(config->node_addr == num_nodes) {
                config->num_neighbors = 1;
                config->neighbor_addrs = malloc(sizeof(mixnet_address));
                config->neighbor_addrs[0] = num_nodes - 1;

            }else{
                config->num_neighbors = 2;
                config->neighbor_addrs = malloc(sizeof(mixnet_address) * 2);
                config->neighbor_addrs[0] = config->node_addr - 1;
                config->neighbor_addrs[1] = config->node_addr + 1;
            }
        } break;

        case FULLY_CONNECTED: {
            config->num_neighbors = num_nodes - 1;
            config->neighbor_addrs = malloc(sizeof(mixnet_address) * (num_nodes-1));
            int j=0;
            for(int i=1; i<=num_nodes; i++){
                if(i != config->node_addr){
                    config->neighbor_addrs[j] = i;
                    j++;
                }
            }
        } break;

        case TREE_4: {
            set_topology_tree_4(config);
        } break;

        case MESH_7: {
            set_topology_mesh_7(config);
        } break;

        case MESH_9: {
            set_topology_mesh_9(config);
        } break;

        default:
            exit(1);
        break;
    }
}

// Blocks or unblocks a node's neighbour's allocated port
void port_to_neighbor(mixnet_node_config_t *config, mixnet_address niegbhor_addr, uint8_t *ports, enum port_decision decision) {
    
    for(int nid=0; nid<config->num_neighbors; nid++){
        if(config->neighbor_addrs[nid] == niegbhor_addr){

            if(decision == BLOCK_PORT)
                ports[nid] = 0;
            else if(decision == OPEN_PORT)
                ports[nid] = 1;      
            break;
        }
    } 
}

void activate_all_ports(uint8_t *ports, int port_len) {
    for(int i=0; i<port_len; i++) {
        ports[i] = 1;
    }
}

void print_ports(mixnet_node_config_t *config, uint8_t *ports) {
    HAL_printf("ports: [ ");
    for(int nid=0; nid<config->num_neighbors; nid++){
        HAL_printf("%u ", config->neighbor_addrs[nid]);
    } 
    HAL_printf("]\n");

    HAL_printf("       [ ");
    for(int nid=0; nid<config->num_neighbors; nid++){
        HAL_printf("%u ", ports[nid]);
    } 
    HAL_printf("]\n");
}

int get_port(mixnet_address *arr, size_t arr_len, mixnet_address data) {
    int result = -1;
    for(int i=0; i<arr_len; i++) {
        if(arr[i] == data) {
        result = i;
        }
    }
    return result;
}

void get_topo_str(enum topology_t topo, int num_nodes, char *topo_str) {
    switch(topo){
        case TREE_4:
            sprintf(topo_str, "TREE_4");
        break;

        case LINE:
            sprintf(topo_str, "LINE_%d", num_nodes);
        break;

        case FULLY_CONNECTED:
            sprintf(topo_str, "FULLY_CONNECTED_%d", num_nodes);
        break;

        default:
        break;
    }
}

void send_ack_pkt(mixnet_node_config_t *config, UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, mixnet_address dst_addr, uint16_t acknum) {
    mixnet_packet_t *pkt = malloc(sizeof(mixnet_packet_t) + sizeof(mixnet_packet_stp_t));
    pkt->src_address = config->node_addr;
    pkt->dst_address = dst_addr;
    pkt->type = PACKET_TYPE_ACK;
    pkt->payload_size = 0;
    pkt->seqnum = 0;
    pkt->acknum = acknum;

    LORA_send_packet(lora_huart, serial_huart, pkt);

    free(pkt);
}

// from Zed 
int myPow(int x, unsigned int p)
{
  if (p == 0) return 1;
  if (p == 1) return x;
  
  int tmp = myPow(x, p/2);
  if (p%2 == 0) return tmp * tmp;
  else return x * tmp * tmp;
}

// wait a radom number in [1-100] of LORA_TOAs
void wait_random(bool reset_exp_backoff) {
    static int iter_count = 0;
    if(reset_exp_backoff){
        iter_count = 0;
    }

    int random_int = rand();
    int rand_wait_time = (random_int % (LORA_TOA + TIME_BUF));
    int exp_backoff = 0;
    
    if(iter_count == 0){
        exp_backoff = 0;
    }else{
        exp_backoff = (myPow(2, iter_count)) * 1000;
    }

    if(rand_wait_time + exp_backoff < MAX_BACKOFF){
        wait_ms(rand_wait_time + exp_backoff);
    }else{
        wait_ms(MAX_BACKOFF);
    }
    iter_count++;
}

// note: these are not for broadcast packets
int rdt_send_pkt(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, mixnet_node_config_t *config, mixnet_packet_t *pkt){
    static uint16_t seqnum=0;
    char *raw_packet;
    mixnet_packet_t *recvd_packet;
    int recv_port = 0;
    bool recvd_ack = false;
    err_code_t err_code;
    bool reset_exp_backoff = true;
    int retx_attempts = 0;
    int ret_val = 0;

    uint32_t start_time, end_time, send_time, elapsed_time;

    while(1) {
        start_time = tim2_tick_count;
        recvd_ack = false;

        #if DBG_RDT
        HAL_printf("[%u] Transmitting packet [SEQ %u] to %u...\n", config->node_addr, seqnum, pkt->dst_address);
        #endif

        pkt->seqnum= seqnum;
        pkt->acknum= 0;
        LORA_send_packet(&huart1, &huart2, pkt);

        send_time = tim2_tick_count - start_time;

        #if DBG_RDT
        HAL_printf("send time: %u\n", send_time);
        #endif

        start_time = tim2_tick_count;

        #if DBG_RDT
        HAL_printf("Waiting for [ACK %u] from %u...\n", pkt->seqnum, pkt->dst_address);
        #endif
        raw_packet = lora_recv_data(&huart1, &huart2, start_time, (LORA_TOA + TIME_BUF), NON_BLOCKING, &err_code);

        if(raw_packet != NULL){
            recvd_packet = (mixnet_packet_t*)raw_packet;
            assert(recvd_packet->dst_address == config->node_addr); // was the packet actually destined to you (not just broadcast)

            recv_port = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
            if(recv_port != -1){
                if(recvd_packet->type == PACKET_TYPE_ACK) {
                    if((recvd_packet->src_address == pkt->dst_address) && 
                        (recvd_packet->acknum == pkt->seqnum)) {
                            #if DBG_RDT
                            HAL_printf("Packet [SEQ %u] was acknoweldged [ACK %u]\n", pkt->seqnum, recvd_packet->acknum);
                            #endif
                            recvd_ack = true;
                    }else{

                        #if DBG_RDT
                        HAL_printf("incorrect ACK\n");
                        #endif
                    }
                }
            }
        
        // either the packet had a CRC error, or the ACK was never received
        }else{
            if(err_code == NO_ERROR){ // case 2: we do NOT receive an ACK - complete timeout, restart checking
                #if DBG_RDT
                HAL_printf("Packet %u timed out after %d retransmissions\n", seqnum, retx_attempts);
                #endif

            }else if(err_code == PKT_ERROR){
                #if DBG_RDT
                HAL_printf("Packet Error in expected ACK after %d retransmissions\n", retx_attempts);
                #endif
                // wait_random(reset_exp_backoff); // random exponential backoff
                // reset_exp_backoff = false;

            }else if(err_code == PKT_ERROR_CRC){
                #if DBG_RDT
                HAL_printf("Packet CRC Error in expected ACK\n");
                #endif
                break;

            }else{
                #if DBG_RDT
                HAL_printf("Uknown error\n");
                #endif
            }
        }

        if(recvd_ack){
            wait_ms((LORA_TOA * 2) + TIME_BUF);
            break;
        }

        if(retx_attempts > MAX_RETX_ATTEMPTS){
            ret_val = -1;
            break;
        }
        retx_attempts++;
    }

    #if DBG_RDT
    HAL_printf("ack time: %u\n", tim2_tick_count - start_time);
    #endif


    free(pkt);
    seqnum++;
    return ret_val;
}

char *rdt_recv_pkt(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, mixnet_node_config_t *config) {
    char *raw_packet;
    mixnet_packet_t *recvd_packet;
    mixnet_packet_t *recorded_packet = NULL;
    int recv_port = 0;
    uint32_t start_time, elapsed_time;
    err_code_t err_code;
    bool is_packet_retx = false;
    uint16_t prev_seqnum;
    mixnet_address prev_src_addr;

    // block until you receive a packet WITHOUT errors
    while(1) {
        start_time = tim2_tick_count;
        #if DBG_RDT
        HAL_printf("[%u] Waiting for any packet...\n", config->node_addr);
        #endif
        
        raw_packet = lora_recv_data(&huart1, &huart2, 0, 0, BLOCKING, &err_code);

        if(raw_packet != NULL){
            start_time = tim2_tick_count;
            recvd_packet = (mixnet_packet_t*)raw_packet;

            recv_port  = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
            if(recv_port != -1) {
                prev_seqnum = recvd_packet->seqnum;
                prev_src_addr = recvd_packet->src_address;
                recorded_packet = recvd_packet;
                break;
            }

        }else{
            if(err_code == NO_ERROR){
                #if DBG_RDT
                HAL_printf("packet timeout\n");
                #endif
            }else{
                #if DBG_RDT
                HAL_printf("Packet Errors in expected packet!\n");
                #endif
            }
        }
    }   

    // at this point, a packet was received without errors
    assert((raw_packet != NULL) && (err_code == NO_ERROR));

    #if DBG_RDT
    HAL_printf("recv time: %u\n", start_time);
    #endif

    send_ack_pkt(config, &huart1, &huart2, recvd_packet->src_address, recvd_packet->seqnum);

    /* WAIT AND ACK */
    start_time = tim2_tick_count;
    HAL_printf("Checking for retransmissions\n");
    raw_packet = lora_recv_data(&huart1, &huart2, start_time, (LORA_TOA + TIME_BUF), NON_BLOCKING, &err_code);
    if(raw_packet != NULL){
        recvd_packet = (mixnet_packet_t*)raw_packet;

        recv_port  = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
        if(recv_port != -1) {
            if(recvd_packet->src_address == prev_src_addr && recvd_packet->seqnum == prev_seqnum){
                HAL_printf("ACK'ing the re-transmission\n");
                send_ack_pkt(config, &huart1, &huart2, recvd_packet->src_address, recvd_packet->seqnum);
            }
        }
    }else{
        if(err_code == NO_ERROR){
            HAL_printf("No retransmissions detected\n");
        }else{
            HAL_printf("Error %u in retransmission\n", err_code);
        }
        wait_ms(LORA_TOA);
    }

    #if DBG_RDT
    HAL_printf("send ack time: %u\n", tim2_tick_count - start_time);
    #endif

    return (char*)recorded_packet;
}


char *rdt_recv_data_pkt(UART_HandleTypeDef * lora_huart, UART_HandleTypeDef * serial_huart, mixnet_node_config_t *config, uint32_t timeout) {
    char *raw_packet;
    mixnet_packet_t *recvd_packet;
    mixnet_packet_t *recorded_packet = NULL;
    int recv_port = 0;
    uint32_t start_time, elapsed_time;
    err_code_t err_code;
    bool is_packet_retx = false;
    uint16_t prev_seqnum;
    mixnet_address prev_src_addr;

    // block until you receive a packet WITHOUT errors
        
    raw_packet = lora_recv_data(&huart1, &huart2, 0, (timeout), NON_BLOCKING, &err_code);

    if(raw_packet != NULL){
        start_time = tim2_tick_count;
        recvd_packet = (mixnet_packet_t*)raw_packet;

        recv_port  = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
        if(recv_port != -1) {
            prev_seqnum = recvd_packet->seqnum;
            prev_src_addr = recvd_packet->src_address;
            recorded_packet = recvd_packet;
            // break;
        }

        // at this point, a packet was received without errors
        send_ack_pkt(config, &huart1, &huart2, recvd_packet->src_address, recvd_packet->seqnum);

        /* WAIT AND ACK */
        start_time = tim2_tick_count;
        HAL_printf("Checking for retransmissions\n");
        raw_packet = lora_recv_data(&huart1, &huart2, start_time, (LORA_TOA + TIME_BUF), NON_BLOCKING, &err_code);
        if(raw_packet != NULL){
            recvd_packet = (mixnet_packet_t*)raw_packet;

            recv_port  = get_port(config->neighbor_addrs, config->num_neighbors, recvd_packet->src_address);
            if(recv_port != -1) {
                if(recvd_packet->src_address == prev_src_addr && recvd_packet->seqnum == prev_seqnum){
                    HAL_printf("ACK'ing the re-transmission\n");
                    send_ack_pkt(config, &huart1, &huart2, recvd_packet->src_address, recvd_packet->seqnum);
                }
            }
        }else{
            if(err_code == NO_ERROR){
                HAL_printf("No retransmissions detected\n");
            }else{
                HAL_printf("Error %u in retransmission\n", err_code);
            }
        }

    }else{
        if(err_code == NO_ERROR){
            HAL_printf("packet timeout\n");
        }else{
            HAL_printf("Packet Errors in expected packet!\n");
        }
    }


    return (char*)recorded_packet;
}
