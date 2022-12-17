
#include <stdio.h>
#include <string.h>
#include "json-c/json.h"
#include "../inc/protocols.h"

extern int NUM_NODES;

void json_write(mixnet_node_config_t *config, graph_t *graph, data_config_t *sensor_db){


    json_object *node0 = json_object_new_array();
    json_object *node1 = json_object_new_array();
    json_object *node2 = json_object_new_array();
    json_object *node3 = json_object_new_array();
    json_object *node4 = json_object_new_array();
    json_object *node5 = json_object_new_array();
    json_object *node6 = json_object_new_array();
    json_object *node7 = json_object_new_array();
    json_object *node8 = json_object_new_array();

    adj_vert_t *curr_indx = graph->head;
    adj_node_t *node;

    while (curr_indx != NULL) {
        node = curr_indx->adj_list;
        while (node != NULL) {
            printf("%u %u \n", curr_indx->addr, node->addr);
            json_object *intj = json_object_new_int(node->addr-1);
            if (curr_indx->addr == 2){
                json_object_array_add(node1, intj);
            }
            else if (curr_indx->addr == 3){
                json_object_array_add(node2, intj);
            }
            else if (curr_indx->addr == 4){
                json_object_array_add(node3, intj);
            }
            else if (curr_indx->addr == 5){
                json_object_array_add(node4, intj);
            }
            else if (curr_indx->addr == 6){
                json_object_array_add(node5, intj);
            }
            else if (curr_indx->addr == 7){
                json_object_array_add(node6, intj);
            }
            else if (curr_indx->addr == 8){
                json_object_array_add(node7, intj);
            }
            else if (curr_indx->addr == 9){
                json_object_array_add(node8, intj);
            }
            else if (curr_indx->addr == 1){
                json_object_array_add(node0, intj);
                printf("test: %u\n",node->addr-1 );
            }
            node = node->next;
        }
        curr_indx = curr_indx->next_vert;
    } 
    json_object *dead = json_object_new_array();
    json_object *fire_nodes = json_object_new_array();
    for (int i = 1;  i < NUM_NODES; i++){
        bool responsive = 0;
        for (int j = 0;  j < sensor_db->num_resp_nodes; j++){
            if (i+1 == sensor_db->resp_nodes[j]){
                responsive = 1;
            }
        }
        if (!responsive){
            json_object *inti = json_object_new_int(i);
            json_object_array_add(dead, inti);
        }
    }
    for (int j = 0; j < sensor_db->num_nodes_on_fire; j++){
        json_object *fire = json_object_new_int(sensor_db->nodes_on_fire[j]-1);
        json_object_array_add(fire_nodes, fire);
        
    }

    json_object * jobj = json_object_new_object();
    json_object * active_links = json_object_new_object();

    json_object_object_add(active_links ,"node0", node0);
    json_object_object_add(active_links ,"node1", node1);
    json_object_object_add(active_links ,"node2", node2);
    json_object_object_add(active_links ,"node3", node3);
    json_object_object_add(active_links ,"node4", node4);
    json_object_object_add(active_links ,"node5", node5);
    json_object_object_add(active_links ,"node6", node6);
    json_object_object_add(active_links ,"node7", node7);
    json_object_object_add(active_links ,"node8", node8);

    json_object_object_add(jobj,"firenode", fire_nodes);
    json_object_object_add(jobj,"deadnodes", dead);
    json_object_object_add(jobj,"activelinks", active_links);


    // Transforms the binary representation into a string representation
    const char* json_string = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_SPACED);


    //print to file
    FILE* fptr;
    fptr = fopen("sample.json", "w");
    fprintf(fptr, "%s", json_string); 
    fclose(fptr);
    

    

}