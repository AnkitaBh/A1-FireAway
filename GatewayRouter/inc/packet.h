/**
  * @file           : packet.h
  * @brief          : Definition for network packet headers (adapted from 15-641 Lab 1)
  * @author         : Arden Diakhate-Palme
  * @date           : 14.12.22
**/

#ifndef _PACKET_H_
#define _PACKET_H_
#include <unistd.h>
#include <assert.h>
#include <stdalign.h>
#include <stdint.h>

typedef uint16_t mixnet_address;
typedef uint16_t timeslot_t;

enum mixnet_packet_type_enum {
    PACKET_TYPE_STP = 0,
    PACKET_TYPE_LSA,
    PACKET_TYPE_SCH,
    PACKET_TYPE_DATA,
    PACKET_TYPE_ACK
};
// Shorter type alias for the enum
typedef uint8_t mixnet_packet_type_t;

enum lsa_flag_type_enum {
    LSA_REQ = 0,
    LSA_RESP = (1 << 1),
    LSA_RESP_END = (1 << 2),
    LSA_INV = (1 << 3)
};
typedef uint8_t lsa_flag_type_t;

enum sch_flag_type_enum {
    SCH_DATA = 0,
    SCH_ACK = (1 << 1)
};
typedef uint8_t sch_flag_type_t;

/// @brief Represents a generic packet header.
typedef struct mixnet_packet {
    mixnet_address src_address;     // source address
    mixnet_address dst_address;     // destination address
    mixnet_packet_type_t type;      // The type of Mixnet packet
    uint16_t seqnum;                 // SEQUENCE number
    uint16_t acknum;                 // ACKNOWLEDGEMENT number
    uint8_t payload_size;           // Payload size (in bytes)
    char payload[];                 // Variable-size payload
} mixnet_packet_t;


/// @brief Represents the payload for an STP packet.
typedef struct __attribute__((__packed__)) mixnet_packet_stp {
    mixnet_address root_address;    // Root of the spanning tree
    uint16_t path_length;           // Length of path to the root
    mixnet_address node_address;    // Current node's mixnet address
} mixnet_packet_stp_t;

/// @brief Represents the payload for an LSA packet.
typedef struct __attribute__((__packed__)) mixnet_packet_lsa {
    mixnet_address node_address;    // Advertising node's mixnet address
    uint16_t neighbor_count;        // Length of path to the root
    lsa_flag_type_t flags;          // flags for what kind of LSA packet
} mixnet_packet_lsa_t;


/// @brief header, followed by the RX timeslots
typedef struct __attribute__((__packed__)) packet_sch {
    uint16_t total_ts;        // total number of timeslots
    timeslot_t tx_ts;
    uint32_t delay_to_phase4;
    uint16_t rxtx_list_len;
    sch_flag_type_t flags;
} packet_sch_t;

/// @brief data packet followed by node addresses where a fire has been detected,
///        then addresses of nodes that have responded 
typedef struct __attribute__((__packed__)) packet_data {
    uint16_t num_nodes_on_fire;
    uint16_t num_resp_nodes;
} packet_data_t;


/// @brief Internal database for the output of STP phase
typedef struct{
    mixnet_address root_address;     // Root of the spanning tree
    uint16_t path_length;            // Length of path to the root  
    mixnet_address next_hop_address; // Next hop on the path to the root   
    mixnet_address stp_parent_addr;  // Next hop on the path to the root   
    uint16_t stp_parent_path_length; // internal update variable
    uint8_t *stp_ports;              // Ports across the spanning tree
} stp_route_t;

/// @brief Configuration information for each node
typedef struct {
    mixnet_address node_addr; // Mixnet address of this node
    uint16_t num_neighbors; // This node's total neighbor count
    mixnet_address *neighbor_addrs; // Mixnet addresses of neighbors
} mixnet_node_config_t;



#endif