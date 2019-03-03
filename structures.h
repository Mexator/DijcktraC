//
// Created by anton on 02.03.19.
//

#ifndef A1_STRUCTURES_H
#define A1_STRUCTURES_H

typedef struct
{
    int cities_number;
    int **ways;
    int initial_city, destination_city;
} graph_info;
enum messages
{
    success, cities_out_of_range,
    initial_error, destination_error,
    matrix_error, dist_out_of_range,
    no_way, structure_mismatch, internal_error
};
typedef struct dijkstra_node dijkstra_node;
struct dijkstra_node
{
    int number;
    int mark;
    dijkstra_node** prev;
    int prev_num;
};
#endif //A1_STRUCTURES_H
