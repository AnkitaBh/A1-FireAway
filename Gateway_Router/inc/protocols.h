/**
  * @file           : protocols.h
  * @brief          : Definitions for networking protocol stack functions (Phases: STP, LSA, SCH, DATA) and networking helper functions.
  * @author         : Arden Diakhate-Palme
  * @date           : 14.12.22
**/

#ifndef _PROTOCOLS_H_
#define _PROTOCOLS_H_
#include <unistd.h>

#include "graph.h"
#include "packet.h"
#include "schedule.h"

#define DBG_STP 1
#define DBG_LSA 1

enum port_decision {BLOCK_PORT= 0, OPEN_PORT};

/// @brief Initial network topology
enum topology_t {
    LINE = 0, 
    FULLY_CONNECTED,
    TREE_4,
    MESH_7,
    MESH_9
};

typedef struct {
    // same number of TS as there are nodes in the network  - 1 (to exclude GW) (b/c each node TX's in a given TS)
    uint16_t total_ts;          //total number of absolute timeslots; 
    //(will have to add this remaining sleep time to the total sleep time after phase 4)
    timeslot_t tx_ts;           // each node only transmits data once back up to its parent
    uint16_t num_rx_ts;
    timeslot_t *rx_ts;          // [0, 1]
    uint16_t num_ts_sleep;      // number of timeslots to sleep for after transmitting
} data_sch_config_t;

typedef struct {
    uint16_t num_nodes_on_fire;
    mixnet_address *nodes_on_fire;
    uint16_t num_resp_nodes;
    mixnet_address *resp_nodes;
} data_config_t;

/* STP Functions */

/// @brief Broadcast an STP message
void broadcast_stp(mixnet_node_config_t *config, stp_route_t *stp_route_db);

/// @brief Updates the STP database based on newly-received information from neighbors
void update_stp_db(mixnet_node_config_t *config,  uint8_t recv_port, stp_route_t *stp_route_db, 
               mixnet_packet_stp_t *recvd_stp_packet);


/* LSA Functions */
/// @brief Querries that node's children nodes to retrieve their neighbors
int get_child_node_lsa(mixnet_node_config_t *config, stp_route_t *stp_route_db, graph_t *net_graph);

/* Functions to update internal graph and send out LSA requests and responses */
void send_lsa_req(mixnet_node_config_t *config, mixnet_address dst_addr);
void send_lsa_inv(mixnet_node_config_t *config, mixnet_address dst_addr);
void send_lsa_resp(mixnet_node_config_t *config, stp_route_t *stp_route_db, graph_t *net_graph);
void add_to_graph(graph_t *net_graph, mixnet_packet_lsa_t *recvd_lsa_pkt);


/* SCH Functions */
void init_sch_config(data_sch_config_t *sch_config);
void reset_sch_config(data_sch_config_t *sch_config);
void populate_sch_config(mixnet_node_config_t * config, schedule_t *sch, data_sch_config_t *sch_config);
void populate_local_sch(schedule_t *local_sch, packet_sch_t *recvd_sch_pkt);
void send_sch(mixnet_node_config_t *config, schedule_t *global_sch, uint32_t delay_to_phase4, mixnet_address dst_node_addr);
void print_sch_config(data_sch_config_t *sch_config);
void print_local_sch(schedule_t *local_sch);

uint16_t get_responsive_child_nodes(mixnet_node_config_t *config, stp_route_t *stp_route_db, graph_t *net_graph, 
                                    mixnet_address *child_nodes, mixnet_address parent_node, mixnet_address node);
void query_sch(mixnet_node_config_t *config, stp_route_t *stp_route_db, schedule_t *node_sch, uint32_t delay_to_phase4);
void send_sch_ack(mixnet_node_config_t *config, mixnet_address dst_addr);

/* DATA Functions */
void send_sensor_data(mixnet_node_config_t *config, data_config_t *sensor_db, mixnet_address dst_node_addr);
void populate_sensor_data(data_config_t *sensor_db, packet_data_t *recvd_data_pkt);
void print_sensor_db(data_config_t *sensor_db);
void init_sensor_db(mixnet_node_config_t *config, data_config_t *sensor_db);
void reset_sensor_db(mixnet_node_config_t *config, data_config_t *sensor_db);

/* Network Helper Functions */

/// @brief configures the node's neighbors/ports to fit in the request topology 
void set_topology(mixnet_node_config_t *config, uint16_t num_nodes, enum topology_t topology);

/// @brief Block or Open a node's port 
void port_to_neighbor(mixnet_node_config_t *config, mixnet_address niegbhor_addr, uint8_t *ports, 
                      enum port_decision decision);

// returns neighbor's port from given address (index into neighbor's array)
int get_port(mixnet_address *arr, size_t arr_len, mixnet_address data);

void print_ports(mixnet_node_config_t *config, uint8_t *ports);
void activate_all_ports(uint8_t *ports, int port_len);
int rdt_send_pkt(mixnet_node_config_t *config, mixnet_packet_t *pkt);
char *rdt_recv_pkt(mixnet_node_config_t *config);
int get_elapsed_time(struct timeval *start_time, struct timeval *end_time);
void send_ack_pkt(mixnet_node_config_t *config, mixnet_address dst_addr, uint16_t acknum);

#endif