#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>
#include "packet.h"

// neighboring nodes of vertex node
struct adjacency_node {
    mixnet_address addr;
    struct adjacency_node *next;
};
typedef struct adjacency_node adj_node_t;

struct adjacency_vert {
    mixnet_address addr;
    adj_node_t *adj_list; // Node's list of neighbors
    uint16_t num_children;
    struct adjacency_vert *next_vert;
};
typedef struct adjacency_vert adj_vert_t;

typedef struct {
    adj_vert_t *head;
    adj_vert_t *tail;
    uint16_t    num_vert;
} graph_t;

graph_t *graph_init(void);

// retrieve adjacency vertex (vertex on primary linked list)
adj_vert_t *get_adj_vertex(graph_t *net_graph, mixnet_address vert_node);

// if a node is not already in the vertex list of the graph, add it and its neighbors
bool graph_add_neighbors(graph_t *net_graph, mixnet_address vert_node, mixnet_address *node_list, uint16_t node_count);

// prints the neighbors of a vertex node
void print_adj_list(adj_node_t* adj);
void print_graph(graph_t *net_graph);

// if a node shows up in another vertex's neighbors list, but is not itself a vertex, add the node as a vertex
void verify_graph(graph_t *net_graph);

// checks if the adjacency list (the neighbors of a specific vertex) has node <node_addr> in its list
bool adj_list_has_node(graph_t *net_graph, adj_vert_t *adj_vertex, mixnet_address node_addr);

// returns the vertex
adj_vert_t *find_vertex(graph_t *net_graph, mixnet_address vert_node);
void remove_tree_neighbor(mixnet_node_config_t *config, graph_t *net_graph, mixnet_address target_addr);

#endif 