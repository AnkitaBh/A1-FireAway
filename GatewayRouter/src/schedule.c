/**
  * @file           : schedule.c
  * @brief          : Functions for creating SCH packets in Phase 3, and for splitting up the RX/TX schedule and storing it locally in RAM
  * @author         : Arden Diakhate-Palme
  *                   Modified by karenabruzzo 
  * @date           : 14.12.22
**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

#include "../inc/schedule.h"

extern int NUM_NODES;

bool node_in_list(mixnet_address *list, uint16_t listsz, mixnet_address addr) {
    for (uint16_t idx = 0; idx < listsz; idx++) {
        if (list[idx] == addr)
            return true;
    }
    return false;
}

void remove_from_list(mixnet_address *list, uint16_t listsz, mixnet_address addr){
    uint16_t idx = 0;
    while (idx < listsz) {
        if (list[idx] == addr) {
            break;
        }
        idx++;
    }

    while (idx < listsz) {
        if (idx < (listsz - 1))
            list[idx] = list[idx+1];
        else 
            list[idx] = 0;
        idx++;
    }
}

void calculate_global_sch(schedule_t *global_sch) {
    global_sch->rx_list[0] = 2;
    global_sch->rx_list[1] = 2;
    global_sch->rx_list[2] = 0;

    global_sch->tx_list[0] = 4;
    global_sch->tx_list[1] = 3;
    global_sch->tx_list[2] = 2;

    global_sch->sch_recv_order[0] = 2;
    global_sch->sch_recv_order[1] = 4;
    global_sch->sch_recv_order[2] = 3;
}

void populate_schedule(graph_t *graph, schedule_t *sch) {
    mixnet_address visited[NUM_NODES];
    mixnet_address to_visit[NUM_NODES];
    uint16_t v_count = 0, to_count = 0;
    uint16_t recv_count = 0;
    uint16_t rxtx_count = NUM_NODES - 2;
    adj_vert_t *adj_vertex;

    /* Start with head and mark it as visited */
    adj_vertex = graph->head;

    to_visit[to_count] = adj_vertex->addr;
    to_count++;

    while(to_count > 0) {
        adj_vertex = find_vertex(graph, to_visit[0]);
        if (adj_vertex == NULL) {
            /* Not a vertex */
                remove_from_list(to_visit, NUM_NODES, to_visit[0]);
                to_count--;
            continue;
        }

        visited[v_count] = adj_vertex->addr;
        v_count++;

        remove_from_list(to_visit, NUM_NODES, adj_vertex->addr);
        to_count--;

        adj_node_t *node = adj_vertex->adj_list;
        while (node != NULL) {
            if (!node_in_list(visited, NUM_NODES, node->addr)) {
                sch->sch_recv_order[recv_count] = node->addr;
                recv_count++;

                sch->rx_list[rxtx_count] = (adj_vertex->addr == 1) ? 0: adj_vertex->addr;
                sch->tx_list[rxtx_count] = node->addr;
                rxtx_count--;
                
                if (!node_in_list(to_visit, NUM_NODES, node->addr)) {
                    to_visit[to_count] = node->addr;
                    to_count++;
                } 
            }
            node = node->next;
        }

    }
}

void init_sch(schedule_t *sch){
    sch->total_ts = NUM_NODES - 1;
    sch->rx_list = calloc((NUM_NODES - 1), sizeof(mixnet_address));
    sch->tx_list = calloc((NUM_NODES - 1), sizeof(mixnet_address));
    sch->sch_recv_order = calloc((NUM_NODES - 1), sizeof(mixnet_address));
}

void reset_sch(schedule_t *sch){
    sch->total_ts = NUM_NODES - 1;
    for(int i=0; i<(NUM_NODES - 1); i++){
        sch->rx_list[i] = 0;
        sch->tx_list[i] = 0;
        sch->sch_recv_order[i] = 0;
    }
}

void print_schedule(schedule_t *local_sch) {
    printf("RX: [ ");
    for(int i=0; i<local_sch->total_ts; i++){
        printf("%u ", local_sch->rx_list[i]);
    }
    printf("]\n");

    printf("TX: [ ");
    for(int i=0; i<local_sch->total_ts; i++){
        printf("%u ", local_sch->tx_list[i]);
    }
    printf("]\n");

    printf("schedule receive order: [ ");
    for(int i=0; i<local_sch->total_ts; i++){
        printf("%u ", local_sch->sch_recv_order[i]);
    }
    printf("]\n");
}

void free_sched(schedule_t *sched) {
     if (sched != NULL) {
        free(sched->sch_recv_order);
        free(sched->rx_list);
        free(sched->tx_list);
        free(sched);
     }
}
