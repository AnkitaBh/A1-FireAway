/**
  * @file           : graph.c
  * @brief          : Definitions for adjacency list graph implementation and helper functions (some funcitons adapted from 15-641 Lab1)
  * @author         : Arden Diakhate-Palme 
  * @date           : 14.12.22
  * 
**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>

#include "graph.h"

graph_t *graph_init(void) {
    graph_t *graph = malloc(sizeof(graph_t));
    graph->head = NULL;
    graph->tail = NULL;
    graph->num_vert = 0;
    return graph;
}

bool is_vertex(graph_t *net_graph, mixnet_address addr) {
    adj_vert_t *tmp_vert = net_graph->head;
    bool ret = false;
    while(tmp_vert != NULL) {
        if(tmp_vert->addr == addr)
            ret = true;
        tmp_vert = tmp_vert->next_vert;
    }
    return ret;
}

void verify_graph(graph_t *net_graph){
    adj_vert_t *adj_vertex =  net_graph->head;
    adj_vert_t *tmp_vert;
    adj_node_t *search_node, *node;

    while(adj_vertex != NULL){
        adj_node_t *tmp_node = adj_vertex->adj_list;
        while(tmp_node != NULL){
            if(!is_vertex(net_graph, tmp_node->addr)){
                tmp_vert = get_adj_vertex(net_graph, tmp_node->addr);

                if(!adj_list_has_node(net_graph, tmp_vert, adj_vertex->addr)) {
                    node = malloc(sizeof(adj_node_t));
                    node->addr = adj_vertex->addr;
                    node->next = NULL;
                    tmp_vert->num_children += 1;
                    
                    //append
                    if(tmp_vert->adj_list == NULL){
                        tmp_vert->adj_list = node;
                    }else{
                        search_node = tmp_vert->adj_list;
                        while(search_node->next != NULL){
                            search_node = search_node->next;
                        }
                        search_node->next = node;
                    }
                }
            }
            tmp_node = tmp_node->next;
        }

        adj_vertex = adj_vertex->next_vert;
    }
    
}

bool graph_add_neighbors(graph_t *net_graph, mixnet_address vert_node, mixnet_address *node_list, uint16_t node_count) {
    adj_vert_t *adj_vertex = get_adj_vertex(net_graph, vert_node);
    adj_node_t *node;
    adj_node_t *search_node;
    bool node_in_graph = false;
    bool added = false;

    for(uint16_t node_idx=0; node_idx < node_count; node_idx++){
        node_in_graph = adj_list_has_node(net_graph, adj_vertex, node_list[node_idx]);
        
        if(!node_in_graph) {
            adj_vertex->num_children += 1;
            added = true;
            node = malloc(sizeof(adj_node_t));
            node->addr = node_list[node_idx];
            node->next = NULL;

            // Insert neighbour at start of vertex adjacency list
            if(adj_vertex->adj_list == NULL){
                adj_vertex->adj_list = node;

            // Insert neighbour at end of vertex adjacency list
            }else{
                search_node = adj_vertex->adj_list;
                while(search_node->next != NULL){
                    search_node = search_node->next;
                }

                search_node->next = node;
            }
        }
    }
    return added;
}

bool adj_list_has_node(graph_t *net_graph, adj_vert_t *adj_vertex, mixnet_address node_addr) {
    adj_node_t *search_node;
    search_node = adj_vertex->adj_list;
    bool found_node = false;

    while(search_node != NULL) {
        if(search_node->addr == node_addr) {
            found_node = true;
            break;
        }
        search_node = search_node->next;
    }
    
    return found_node;
}

adj_vert_t *get_adj_vertex(graph_t *net_graph, mixnet_address vert_node) {
    adj_vert_t *adj_vertex = NULL;
    adj_vert_t *search_vert; 

    // no ele in vert list
    if(net_graph->head == NULL && net_graph->tail == NULL && net_graph->num_vert == 0) {
        adj_vertex = malloc(sizeof(adj_vert_t));
        adj_vertex->addr = vert_node;
        adj_vertex->num_children = 0;
        adj_vertex->adj_list = NULL;
        adj_vertex->next_vert = NULL;

        net_graph->head = adj_vertex;
        net_graph->tail = adj_vertex;
        net_graph->num_vert++;

    // one ele in vert list
    }else if(net_graph->head == net_graph->tail && net_graph->num_vert == 1) {

        // is the vertex we're looking for, the only one that is in the vertex list?
        if(net_graph->head->addr == vert_node) {
            adj_vertex = net_graph->head;

        }else{
            adj_vertex = malloc(sizeof(adj_vert_t));
            adj_vertex->addr = vert_node;
            adj_vertex->num_children = 0;
            adj_vertex->adj_list = NULL;
            adj_vertex->next_vert = NULL;
            
            net_graph->tail = adj_vertex;
            net_graph->head->next_vert = adj_vertex;
            net_graph->num_vert++;
        }
        
    // two or more ele in vert list
    }else{
        search_vert = find_vertex(net_graph, vert_node);
        if(search_vert != NULL && search_vert->addr == vert_node) {
            adj_vertex = search_vert;
        }else{
            adj_vertex = malloc(sizeof(adj_vert_t));
            adj_vertex->addr = vert_node;
            adj_vertex->num_children = 0;
            adj_vertex->adj_list = NULL;
            adj_vertex->next_vert = NULL;

            net_graph->tail->next_vert = adj_vertex;        
            net_graph->tail = adj_vertex;  
            net_graph->num_vert++;      
        }
    }
    return adj_vertex;
}

adj_vert_t *find_vertex(graph_t *net_graph, mixnet_address vert_node) {
    adj_vert_t *search_vert = net_graph->head;

    while(search_vert != NULL){
        if(search_vert->addr == vert_node) {
            return search_vert;
        }
        search_vert = search_vert->next_vert;
    }
    return NULL;
}

void print_adj_list(adj_node_t* adj){
    adj_node_t* curr = adj;

    printf("[ ");

    while(curr != NULL){
        printf("%u ", curr->addr);

        curr = curr->next;
    }
    printf("]\n");
}

void print_graph(graph_t *net_graph) {
    adj_vert_t *vert = net_graph->head;
    adj_node_t *node;

    while(vert != NULL) {
        node = vert->adj_list;
        printf("%u: [ ", vert->addr);
        while(node != NULL){
            printf("%u ", node->addr);
            node = node->next;
        }

        printf("]; (%u)\n",vert->num_children);
        vert = vert->next_vert; 
    }
}

void remove_vertex(graph_t *net_graph, mixnet_address target_addr) {
    adj_vert_t *vert = find_vertex(net_graph, target_addr);
    adj_vert_t *tmp_vert, *sec_last_vert;
    adj_vert_t *prev_vert, *next_vert;

    if (vert == NULL) {
        return;
    }

    net_graph->num_vert--;

    if(vert == net_graph->head){
        tmp_vert = net_graph->head;
        net_graph->head = net_graph->head->next_vert;
        free(tmp_vert->adj_list);
        free(tmp_vert);

    }else if(vert == net_graph->tail){
        sec_last_vert = net_graph->head;                
        while(sec_last_vert != NULL){
            if(sec_last_vert->next_vert == net_graph->tail){
                break;
            }
            sec_last_vert= sec_last_vert->next_vert;
        }
        
        sec_last_vert->next_vert = NULL;
        free(net_graph->tail->adj_list);
        free(net_graph->tail);

        net_graph->tail = sec_last_vert;
    }else{
        prev_vert = net_graph->head;
        while(prev_vert != NULL){
            if(prev_vert->next_vert == vert){
                break;
            }
            prev_vert= prev_vert->next_vert;
        }

        next_vert = vert->next_vert;

        prev_vert->next_vert = next_vert;

        free(vert->adj_list);
        free(vert);
    }
}

void remove_adj_node(adj_vert_t *vert, mixnet_address target_addr) {
    adj_node_t *node;
    adj_node_t *prev_node;

    node = vert->adj_list;
    prev_node = NULL;

    while(node != NULL){
        if(node->addr == target_addr){
            // head
            if(node == vert->adj_list){
                vert->adj_list = node->next;
                break;

            //tail
            } else if(node->next == NULL) {
                if (prev_node != NULL) {
                    prev_node->next = NULL;
                    break;
                }

            }else{
                prev_node->next = node->next;
                break;
            }
        }

        prev_node = node;
        node = node->next;
    }

    if (node != NULL) {
        free(node);
        vert->num_children--;
    }
}

void remove_tree_neighbor(mixnet_node_config_t *config, graph_t *net_graph, mixnet_address target_addr) {
    adj_vert_t *vert;

    vert = net_graph->head;
    while(vert != NULL) {
        if(vert->addr == target_addr){
            // remove the whole vertex
            remove_vertex(net_graph, vert->addr);
        }else{
            remove_adj_node(vert, target_addr);
        }
        vert = vert->next_vert; 
    }
}

void free_graph(graph_t *net_graph) {
    adj_vert_t *vert = net_graph->head;
    adj_node_t *node;

    adj_vert_t *tmp_vert;
    adj_node_t *tmp_node;
    while(vert != NULL) {
        node = vert->adj_list;
        while(node != NULL){
            tmp_node = node;
            node = node->next;
            free(tmp_node);
        }
        tmp_vert = vert;
        vert = vert->next_vert; 
        free(tmp_vert);
    }
    free(net_graph);
}