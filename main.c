#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "structures.h"

#define MAX_FIRST_STRING 1024
#define MAX_NUMBER_LENGTH 5
#define MAX_LENGTH 20
#define MIN_LENGTH  1
#define MIN_CITIES 5
#define MAX_CITIES 50

enum messages do_input(FILE *source, graph_info *dest);

void relax(dijkstra_node *node, dijkstra_node *prev, int weight);

enum messages search_ways(graph_info graph, dijkstra_node *output);

void produce_output(FILE *out, dijkstra_node node, int initial_city, enum messages error);

void free_graph_info(graph_info *g);

int main()
{
    //Contains data about the graph gotten from input
    graph_info graph;
    //Contains destination node and all data for backtracking
    dijkstra_node last_node;

    FILE *input_file = fopen("input.txt", "r");
    if (input_file == NULL)
    {
        fprintf(stderr, "Unable to open input file\n");
        return 1;
    }

    FILE *output_file = fopen("AntonBrisilinOutput.txt", "w");
    if (output_file == NULL)
    {
        fprintf(stderr, "Unable to open output file\n");
        return 1;
    }

    enum messages error = do_input(input_file, &graph);
    if (!error)
        error = search_ways(graph, &last_node);

    produce_output(output_file, last_node, graph.initial_city, error);

    free_graph_info(&graph);
}

/**
 * Doing all inputs from {source} and store read info in {dest}
 * @param dest Pointer to {graph_info} structure. Output will be stored in it
 * @return element from {messages} that corresponds to one of errors or success
 */
enum messages do_input(FILE *source, graph_info *dest)
{
    //regex for number that satisfies necessary conditions
    //such as:
    //only decimal digits
    //minus sign is allowed
    //no leading zeros allowed
    //any length of number
    char number_template[] = "-?(0|[1-9]\\d*)";
    char end_of_line[] = "[\n^ ]$";
    //First string
    //Creating regex for 1st string
    regex_t first_string;
    char *first_template = malloc((strlen(number_template) + 1) * 3 + 1);
    if (first_template == NULL)
        return internal_error;
    first_template[0] = '\0';
    sprintf(first_template, "%s %s %s\n", number_template, number_template, number_template);
    if (regcomp(&first_string, first_template, REG_EXTENDED) != 0)
    {
        return internal_error;
    }
    //Reading first string
    char s[MAX_FIRST_STRING];
    fgets(s, MAX_FIRST_STRING, source);

    if (regexec(&first_string, s, 0, NULL, 0) != 0)
    {
        regfree(&first_string);
        return structure_mismatch;
    } else
        sscanf(s, "%d %d %d", &dest->cities_number, &dest->initial_city, &dest->destination_city);
    regfree(&first_string);
    free(first_template);
    if (dest->cities_number < MIN_CITIES || dest->cities_number > MAX_CITIES)
        return cities_out_of_range;
    if (dest->initial_city < 0 || dest->initial_city > dest->cities_number - 1)
        return initial_error;
    if (dest->destination_city < 0 || dest->destination_city > dest->cities_number - 1)
        return destination_error;
    //End of processing first string

    //Empty string
    regex_t empty_string;
    if (regcomp(&empty_string, "^\n", REG_EXTENDED) != 0)
    {
        return internal_error;
    }
    char tmp[2];
    fgets(tmp, 2, source);
    if (regexec(&empty_string, tmp, 0, NULL, 0) != 0)
    {
        regfree(&empty_string);
        return structure_mismatch;
    }
    regfree(&empty_string);
    //

    //Matrix

    //Allocating space for storing strings of adjacency matrix
    dest->ways = calloc(dest->cities_number, sizeof(int *));

    //Creating regex for matrix string
    const int maxline = (MAX_NUMBER_LENGTH + 1) * dest->cities_number + 1;
    regex_t matrix_string;

    char *number_or_star = malloc(strlen(number_template) + strlen("(|\\*)") + 1);
    number_or_star[0] = '(';
    number_or_star[1] = '\0';
    strcat(number_or_star, number_template);
    strcat(number_or_star, "|\\*)");

    char *matrix_str_template = malloc(dest->cities_number * (strlen(number_or_star) + 1) + strlen(end_of_line));
    matrix_str_template[0] = '\0';
    strcat(matrix_str_template, number_or_star);
    for (int i = 0; i < dest->cities_number - 1; ++i)
    {
        strcat(matrix_str_template, " ");
        strcat(matrix_str_template, number_or_star);
    }
    strcat(matrix_str_template, end_of_line);
    if (regcomp(&matrix_string, matrix_str_template, REG_EXTENDED))
        return internal_error;
    free(matrix_str_template);
    free(number_or_star);
    //Reading
    for (int i = 0; i < dest->cities_number; i++)
    {
        char buffer[maxline];
        //Getting string
        fgets(buffer, maxline, source);

        //Checking for structural errors
        if (regexec(&matrix_string, buffer, 0, NULL, 0) != 0)
        {
            regfree(&matrix_string);
            return structure_mismatch;
        } else
        {
            dest->ways[i] = calloc((size_t) dest->cities_number, sizeof(int));
            //Splitting string into tokens
            char *token = strtok(buffer, " \n");
            for (int j = 0; j < dest->cities_number; ++j)
            {
                if (token == NULL)
                    return matrix_error;
                if (strcmp(token, "*") == 0)
                {
                    dest->ways[i][j] = -1;
                } else
                    dest->ways[i][j] = strtol(token, NULL, 10);
                //Checking distances for out of range
                if (i == j)
                {
                    if (dest->ways[i][j] != 0)
                    {
                        return dist_out_of_range;
                    }
                } else
                {
                    if (dest->ways[i][j] < MIN_LENGTH || dest->ways[i][j] > MAX_LENGTH)
                    {
                        if (dest->ways[i][j] != -1)
                            return dist_out_of_range;
                    }
                }
                //Getting next token
                token = strtok(NULL, " \n");
            }
            if (token != NULL)
                return matrix_error;
        }
    }
    regfree(&matrix_string);
    return success;
}

/**
 * Doing Dijkstra search in {graph}. Result is {dijkstra_node} that can be then backtracked
 * @param graph The graph where way need to be found
 * @return element from {messages} that corresponds to one of errors or success
 */
enum messages search_ways(graph_info graph, dijkstra_node *output)
{
    dijkstra_node *unprocessed[graph.cities_number], *dest;
    int unproc_number = graph.cities_number;
    for (int i = 0; i < graph.cities_number; i++)
    {
        unprocessed[i] = malloc(sizeof(dijkstra_node));
        unprocessed[i]->number = i;
        unprocessed[i]->mark = -1;
        unprocessed[i]->prev = malloc(sizeof(dijkstra_node *));
        unprocessed[i]->prev_num = 0;
    }
    unprocessed[graph.initial_city]->mark = 0;
    dest = unprocessed[graph.destination_city];

    dijkstra_node *current = unprocessed[graph.initial_city];

    //While either all are processed
    //or we can not choose appropriate vertex for next step
    while (unproc_number != 0 && current->mark != -1)
    {
        //Do relaxation for adjacent vertexes
        for (int i = 0; i < graph.cities_number; i++)
        {
            if (graph.ways[i][current->number] > 0 && unprocessed[i] != NULL)
            {
                relax(unprocessed[i], current, graph.ways[i][current->number]);
            }
        }
        //choose next vertex
        unprocessed[current->number] = NULL;
        int min = -1;
        for (int i = 0; i < graph.cities_number; ++i)
        {
            if (unprocessed[i] != NULL)
            {
                if ((unprocessed[i]->mark < min && unprocessed[i]->mark != -1) || min == -1)
                {
                    min = unprocessed[i]->mark;
                    current = unprocessed[i];
                }
            }
        }
        unproc_number--;
    }
    if (dest->mark == -1)
        return no_way;
    else
    {
        *output = *dest;
        return success;
    }
}
/**
 * Does relaxation for node {node}
 * @param node The node to be relaxed
 * @param prev The parental node for {node}
 * @param weight Weight of way from {prev} to {node}
 */
inline void relax(dijkstra_node *node, dijkstra_node *prev, int weight)
{
    if (node->mark == -1 || (prev->mark + weight < node->mark))
    {
        node->prev_num = 1;
        node->mark = prev->mark + weight;
        node->prev[0] = prev;
    } else
    {
        if (prev->mark + weight == node->mark)
        {
            node->prev[node->prev_num] = malloc(sizeof(dijkstra_node));
            node->prev[node->prev_num] = prev;
            node->prev_num++;
        }
    }
}
/**
 * Produces array of strings each of that is
 * shortest way from initial node to destination one
 * in appropriate form
 * @param initial_city Number of initial node
 * @param end Number of destination node
 * @param number Length of output array
 * @return Array of strings with ways
 */
char **backtracking(int initial_city, dijkstra_node end, int *number)
{
    dijkstra_node cur = end;
    char **output_array = malloc(sizeof(char *));
    *number = -1;
    for (int i = 0; i < cur.prev_num; i++)
    {
        int subnum;
        char **sub = backtracking(initial_city, *(cur.prev[i]), &subnum);

        for (int j = 0; j < subnum + 1; ++j)
        {
            (*number)++;
            output_array[*number] = calloc(MAX_CITIES + (MAX_CITIES - 1) * 2, sizeof(char));
            strcat(output_array[*number], sub[j]);

            strcat(output_array[*number], " -> ");
            char *tmp = malloc(sizeof(char) * 4);
            sprintf(tmp, "%d", cur.number);
            strcat(output_array[*number], tmp);
        }
        free(sub);
    }
    if (cur.number == initial_city)
    {
        *number = 0;
        output_array[*number] = calloc(MAX_CITIES + (MAX_CITIES - 1) * 2, sizeof(char));
        sprintf(output_array[0], "%d", cur.number);
    }
    return output_array;
}
/**
 * Prints output to {out}
 * @param out The file where to print all outputs
 * @param node The note for doing backtracking if needed
 * @param initial_city number of initial city (needed for backtracking)
 * @param error any error arised during execution (if any)
 */
void produce_output(FILE *out, dijkstra_node node, int initial_city, enum messages error)
{
    switch (error)
    {
        case internal_error:
            fprintf(out, "Sorry, it seems like there is an error in the program\n");
            break;
        case cities_out_of_range:
            fprintf(out, "Number of cities is out of range\n");
            break;
        case initial_error:
            fprintf(out, "Chosen initial city does not exist\n");
            break;
        case destination_error:
            fprintf(out, "Chosen destination city does not exist\n");
            break;
        case matrix_error:
            fprintf(out, "Matrix size does not suit to the number of cities\n");
            break;
        case dist_out_of_range:
            fprintf(out, "The distance between some cities is out of range\n");
            break;
        case no_way:
            fprintf(out, "Initial and destination cities are not connected\n");
            break;
        case structure_mismatch:
            fprintf(out, "Structure of the input is invalid\n");
            break;
        case success:
            fprintf(out, "The shortest path is %d.\n", node.mark);
            int k = 0;
            char **f = backtracking(initial_city, node, &k);
            fprintf(out, "The number of shortest paths is %d:\n", k + 1);
            for (int i = 0; i <= k; i++)
            {
                fprintf(out, "%d. %s", i + 1, f[i]);
                if (i < k + 1)
                    fprintf(out, "\n");
            }
            break;
        default:
            fprintf(out, "Unknown error\n");
            break;
    }
}

void free_graph_info(graph_info *g)
{
    for (int i = 0; i < g->cities_number; ++i)
    {
        free(g->ways[i]);
    }
    free(g->ways);
}
