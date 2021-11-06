/***************************************************** 
 * Note: this file should NOT be modified by students. 
 ****************************************************/
#ifndef TASKS_H_

#define TASKS_H_
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef struct _KeyValue {
    char key[8];
    int val;
} KeyValue;

typedef struct _MapTaskOutput {
    int len;                // Length of `kvs` array
    KeyValue *kvs;          // Array of KeyValue items
} MapTaskOutput;

MapTaskOutput* map1(char *file_contents);
MapTaskOutput* map2(char *file_contents);
MapTaskOutput* map3(char *file_contents);
void free_map_task_output(MapTaskOutput *output);
KeyValue reduce(char key[8], int *vals, int len);

#endif 
