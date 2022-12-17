#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdbool.h>

#include "graph.h"
#include "packet.h"

typedef struct {
    uint16_t total_ts; // number of absolute TS -- should be a node TX'ing @ every timeslot
    mixnet_address *tx_list; // [4, 3, 2] 
    mixnet_address *rx_list; // [2, 2]
    mixnet_address *sch_recv_order; // [2 3 4] -- length of array is num_ts 
} schedule_t;

void init_sch(schedule_t *sch);
void calculate_global_sch(schedule_t *global_sch);

// TRANSMIT
void populate_schedule(graph_t *graph, schedule_t *sch);

void print_schedule(schedule_t *sched);

void free_sched(schedule_t *sched);


#endif /* SCHEDULE_H */

