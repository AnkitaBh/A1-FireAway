output: schedule.c

global schedule:

1
| 
2 -- 3
|
4

Note: 1 is the GW and is always in RX,
       rightmost node receives schedule first

RX: 2 2
TX: 4 3 2
TS: - - -  (timeslot when you're running the schedule in phase 4)

schedule receipt ordering: node 1, node 2,...

typedef struct {
    uint16_t num_ts; // number of absolute TS 
    mixnet_address *tx_ts; // [4, 3, 2] 
    mixnet_address *rx_ts; // [2, 2]
    mixnet_address *sch_recv_order; // [2 3 4]
} global_schedule_t;


Phase 3
Ankita
graph.c/.h, packet.h
goal: given the tree graph (adj-list) how many TX slots and RX slots to send to each node in the tree 
 - timeslot index for each slot [1, N]
 - convert UART prints to printf's
 - verify includes
 - structure print function

1
| 
2 -- 3
|
4

// e.g. node 2
typedef struct {
    uint16_t total_abs_ts;        //total number of absolute timeslots; (will have to add this remaining sleep time to the total sleep time after phase 4)
    uint16_t num_abs_rx_ts;       
    uint16_t abs_tx_ts;           // [2]

    uint16_t num_abs_rx_ts;
    uint16_t abs_rx_ts;           // [0, 1]
} sch_db_t;


output: main.c 
Phase 3
Arden 
goal: given a global schedule format, distribute the schedule across the nodes in the tree
 - assigning TX slots to your children nodes that correspond with your RX slots
 - respecting the tree structure with port-masking
 - storing TX slots and RX slots in node DB 
 - wait a certain amount of time (depending on when you receive it), s.t. all nodes start Phase 4 at the same time


1
| 
2 -- 3
|
4

// e.g. node 2
// Data to send in phase 4 
typedef struct {
    uint16_t num_nodes_on_fire;
    mixnet_address *nodes_on_fire;

    mixnet_address *responsive_nbor_nodes;   // if you don't hear from node 4 in abs TS 0, assume it is offline
    uint16_t num_responsive_nbor_nodes;
} temp_data_t;

output: main.c
Phase 4 (non-gateway nodes)
Karen
goal: given TX and RX slots in a local node DB, 
- aggregate information from your children nodes in the RX slots (max of all the temp sensor readings from children nodes)
    - you mention if you miss a reading from your child node (in your next TX slot)
- transmit this information to parent node in the TX slot

======= Notes: =======
- if a node goes offline you need to bring it back online at the 30min mark, when the STP is re-rerun, 
but the guy bringing the node back online, would need to reset it within 3-5 second window

- batteries min 7V, purchase 8x 9V batteries (Duracell)
- battery connection harness (adafruit)

- expansion port (for 4 other nodes) 

- purchase spare parts (LoRa transceivers, temp sensors)

Make sure that before you upload in a new STM32 Cube IDE, you must read launch configs and not autogenerate them
