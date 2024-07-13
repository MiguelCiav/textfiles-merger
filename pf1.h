#ifndef _SORTMERGE_H
#define _SORTMERGE_H

// typedef

typedef char* string;

typedef struct {
    char* filename;
    int id;
} params_t;

typedef struct {
    int number_of_lines;
    string longest_line;
    string shortest_line;
} stats_t;

// API's

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#endif