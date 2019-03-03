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

enum messages do_input(FILE *from, graph_info *input);
void relax(dijkstra_node *node, dijkstra_node *prev, int weight);
enum messages search_ways(graph_info *graph, dijkstra_node *output);
void produce_output(FILE *where, dijkstra_node what, graph_info graph, enum messages error);

int main()
{
    graph_info in;
    dijkstra_node out;
    FILE* file = fopen("in","r");
    enum messages error = do_input(file, &in);
    if (!error)
        error = search_ways(&in, &out);

    produce_output(stdout, out, in, error);
    int i = 0;
    //TODO free
    //TODO test 21, 23
}

/**
 * Doing all inputs for structure
 * @param input
 * @return
 * 0 if input was successful,
 * 1 if Number of cities is out of range
 * 2 if Chosen initial city does not exist
 * 3 if Chosen destination city does not exist
 * 4 if Matrix size does not suit to the number of cities
 * 5 if The distance between some cities is out of range
 * 7 if Structure of the input is invalid
 */
enum messages do_input(FILE *from, graph_info *input)
{
    char number_template[] = "-?(0|[1-9][0-9]*)";
    //First string
    regex_t first_string;
    char *first_template = calloc(1, (strlen(number_template) + 1) * 3 + 1);
    sprintf(first_template, "%s %s %s\n", number_template, number_template, number_template);
    if (regcomp(&first_string, first_template, REG_EXTENDED) != 0)
    {
        return internal_error;
    }

    char s[MAX_FIRST_STRING];
    fgets(s, MAX_FIRST_STRING, from);

    if (regexec(&first_string, s, 0, NULL, 0) != 0)
    {
        regfree(&first_string);
        return structure_mismatch;
    } else
        sscanf(s, "%d %d %d", &input->cities_number, &input->initial_city, &input->destination_city);
    regfree(&first_string);

    if (input->cities_number < MIN_CITIES || input->cities_number > MAX_CITIES)
        return cities_out_of_range;
    if (input->initial_city < 0 || input->initial_city > input->cities_number - 1)
        return initial_error;
    if (input->destination_city < 0 || input->destination_city > input->cities_number - 1)
        return destination_error;
    //

    //Empty string
    regex_t empty_string;
    if (regcomp(&empty_string, "\n", REG_EXTENDED) != 0)
    {
        return internal_error;
    }
    char tmp[2];
    fgets(tmp, 2, from);
    if (regexec(&empty_string, tmp, 0, NULL, 0) != 0)
    {
        regfree(&empty_string);
        return structure_mismatch;
    }
    regfree(&empty_string);
    //

    //Matrix

    //Allocating space
    input->ways = calloc(input->cities_number, sizeof(int *));
    for (int i = 0; i < input->cities_number; i++)
    {
        input->ways[i] = calloc((size_t) input->cities_number, sizeof(int));
    }

    //Reading
    const int maxline = (MAX_NUMBER_LENGTH + 1) * input->cities_number + 1;
    regex_t matrix_string;

    char *number_or_star = calloc(1, strlen(number_template) + strlen("(|\\*)") + 1);
    number_or_star[0] = '(';
    strcat(number_or_star, number_template);
    strcat(number_or_star, "|\\*)");

    char *matrix_str_template = calloc(1, input->cities_number * (strlen(number_or_star) + 1)+strlen("[\n^ ]$"));
    strcat(matrix_str_template, number_or_star);
    for (int i = 0; i < input->cities_number - 1; ++i)
    {
        strcat(matrix_str_template, " ");
        strcat(matrix_str_template, number_or_star);
    }
    strcat(matrix_str_template, "[\n^ ]$");
    if (regcomp(&matrix_string, matrix_str_template, REG_EXTENDED))
        return internal_error;

    for (int i = 0; i < input->cities_number; i++)
    {
        char buffer[maxline];
        fgets(buffer, maxline, from);

        if (regexec(&matrix_string, buffer, 0, NULL, 0))
        {
            regfree(&matrix_string);
            return structure_mismatch;
        } else
        {
            char *token;
            token = strtok(buffer, " \n");
            for (int j = 0; j < input->cities_number; ++j)
            {
                if (token == NULL)
                    return matrix_error;
                if (strcmp(token, "*") == 0)
                {
                    input->ways[i][j] = -1;
                } else
                    input->ways[i][j] = strtol(token, NULL, 10);
                if (i == j)
                {
                    if (input->ways[i][j] != 0)
                    {
                        return dist_out_of_range;
                    }
                } else
                {
                    if (input->ways[i][j] < MIN_LENGTH || input->ways[i][j] > MAX_LENGTH)
                    {
                        if (input->ways[i][j] != -1)
                            return dist_out_of_range;
                    }
                }

                token = strtok(NULL, " \n");
            }
            if (token != NULL)
                return matrix_error;
        }
    }
    regfree(&matrix_string);
    return success;
    //TODO Free
}

/**
 * Doing Dijkstra
 * @param input
 * @return
 * 0 if everything was successful
 * 6 if Initial and destination cities are not connected
 */
enum messages search_ways(graph_info *graph, dijkstra_node *output)
{
    dijkstra_node *unprocessed[graph->cities_number], *all_nodes[graph->cities_number];
    int unproc_number = graph->cities_number;
    for (int i = 0; i < graph->cities_number; i++)
    {
        unprocessed[i] = malloc(sizeof(dijkstra_node));
        unprocessed[i]->number = i;
        unprocessed[i]->mark = -1;
        unprocessed[i]->prev = malloc(sizeof(dijkstra_node *));
        unprocessed[i]->prev_num = 0;
        all_nodes[i] = unprocessed[i];
    }
    unprocessed[graph->initial_city]->mark = 0;

    dijkstra_node *current = unprocessed[graph->initial_city];
    while (unproc_number != 0 && current->mark != -1)
    {
        //релаксируем для смежных
        for (int i = 0; i < graph->cities_number; i++)
        {
            if (graph->ways[i][current->number] > 0 && unprocessed[i] != NULL)
            {
                relax(unprocessed[i], current, graph->ways[i][current->number]);
            }
        }
        //choose next vertex
        unprocessed[current->number] = NULL;
        int min = -1;
        for (int i = 0; i < graph->cities_number; ++i)
        {
            if (unprocessed[i] != NULL && ((unprocessed[i]->mark < min && unprocessed[i]->mark != -1) || min == -1))
            {
                min = unprocessed[i]->mark;
                current = unprocessed[i];
            }
        }
        unproc_number--;
    }
    if (all_nodes[graph->destination_city]->mark == -1)
        return no_way;
    else
    {
        *output = *all_nodes[graph->destination_city];
        return success;
    }
    //TODO Free
}

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
            node->prev[node->prev_num] = prev;
            node->prev_num++;
        }
    }
}

char **backtracking(graph_info graph, dijkstra_node end, int *number)
{
    dijkstra_node cur = end;
    char **output_array = malloc(sizeof(char *));
    *number=-1;
    for (int i = 0; i < cur.prev_num; i++)
    {
        int subnum;
        char** sub = backtracking(graph, *(cur.prev[i]), &subnum);

        for (int j = 0; j < subnum+1; ++j)
        {
            (*number)++;
            output_array[*number] = calloc(MAX_CITIES + (MAX_CITIES - 1) * 2, sizeof(char));
            strcat(output_array[*number], sub[j]);

            strcat(output_array[*number], " -> ");
            char *tmp = malloc(sizeof(char) * 4);
            sprintf(tmp, "%d", cur.number);
            strcat(output_array[*number], tmp);
        }
    }
    if (cur.number == graph.initial_city)
    {
        *number=0;
        output_array[*number] = calloc(MAX_CITIES + (MAX_CITIES - 1) * 2, sizeof(char));
        sprintf(output_array[0], "%d", cur.number);
    }
    return output_array;
}

void produce_output(FILE *where, dijkstra_node what, graph_info graph, enum messages error)
{
    switch (error)
    {
        case internal_error:
            fprintf(where, "Sorry, it seems like there is an error in the program\n");
            break;
        case cities_out_of_range:
            fprintf(where, "Number of cities is out of range\n");
            break;
        case initial_error:
            fprintf(where, "Chosen initial city does not exist\n");
            break;
        case destination_error:
            fprintf(where, "Chosen destination city does not exist\n");
            break;
        case matrix_error:
            fprintf(where, "Matrix size does not suit to the number of cities\n");
            break;
        case dist_out_of_range:
            fprintf(where, "The distance between some cities is out of range\n");
            break;
        case no_way:
            fprintf(where, "Initial and destination cities are not connected\n");
            break;
        case structure_mismatch:
            fprintf(where, "Structure of the input is invalid\n");
            break;
        default:
            fprintf(where, "The shortest path is %d.\n", what.mark);
            int k = 0;
            char **f = backtracking(graph, what, &k);
            fprintf(where, "The number of shortest paths is %d:\n", k+1);
            for (int i = 0; i < k+1; i++)
            {
                fprintf(stdout, "%d. %s", i + 1, f[i]);
                if (i < k)
                    printf("\n");
            }
    }
}