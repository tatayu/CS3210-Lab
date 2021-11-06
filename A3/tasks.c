/***************************************************** 
 * Note: this file should NOT be modified by students. 
 ****************************************************/
#include "tasks.h"

/* Map task to count the number of letters (i.e. alphabets),
 * numbers and other characters (i.e. non-alphanumeric) in 
 * `file_contents`.
 *
 * Note: remember to free the returned MapTaskOutput* with
 * `free_map_task_output`
 * */
MapTaskOutput* map1(char *file_contents) {
    int len = strlen(file_contents);
    int num_letters = 0;
    int num_numbers = 0;
    int num_others = 0;

    int i;
    for (i = 0; i < len; i++) {
        char c = file_contents[i];
        if (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))) {
            num_letters++;
        } else if ((c >= '0') && (c <= '9')) {
            num_numbers++;
        } else {
            num_others++;
        }
    }

    MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
    KeyValue *kvs = (KeyValue *)malloc(sizeof(KeyValue) * 3);
    kvs[0] = (KeyValue) { .key="letters", .val=num_letters };
    kvs[1] = (KeyValue) { .key="numbers", .val=num_numbers };
    kvs[2] = (KeyValue) { .key="others", .val=num_others };

    output->len = 3;
    output->kvs = kvs;

    // Busy waiting (to make map task run longer)
    i = 0;
    while (i < INT_MAX) { i++; }

    return output;
}

/* Map task to count the number of times each alphabet occurs in
 * `file_contents`.
 *
 * Note: remember to free the returned MapTaskOutput* with
 * `free_map_task_output`
 * */
MapTaskOutput* map2(char *file_contents) {
    int len = strlen(file_contents);
    MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
    KeyValue *kvs = (KeyValue *)malloc(sizeof(KeyValue) * 26);

    output->len = 26;
    output->kvs = kvs;

    // Initialise KeyValue structs
    int i;
    for (i = 0; i < 26; i++) {
        char alphabet[2];
        alphabet[0] = 'a' + i;
        alphabet[1] = '\0';
        strcpy((kvs+i)->key, alphabet);
        (kvs+i)->val = 0;
    }

    // Go through each character in `file_contents`
    for (i = 0; i < len; i++) {
        char c = file_contents[i];
        if ((c >= 'A') && (c <= 'Z')) {
            int index = c - 'A';
            (kvs+index)->val += 1;
        } else if ((c >= 'a') && (c <= 'z')) {
            int index = c - 'a';
            (kvs+index)->val += 1;
        }
    }

    // Busy waiting (to make map task run longer)
    i = 0;
    while (i < INT_MAX) { i++; }

    return output;
}


/* Map task that generates a number of KeyValues with the keys
 * "we", "love", "cs", "3210". The total number of KeyValues generated
 * is a function of the length of `file_contents`.
 *
 * Note: remember to free the returned MapTaskOutput* with
 * `free_map_task_output`
 * */
MapTaskOutput* map3(char *file_contents) {
    int len = strlen(file_contents);
    int count = len % 49;

    MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
    KeyValue *kvs = (KeyValue *)malloc(sizeof(KeyValue) * 4 * count);

    output->len = 4 * count;
    output->kvs = kvs;

    int i;
    for (i = 0; i < 4 * count; i++) {
        KeyValue *kv = kvs+i;
        switch(i%4) {
            case 0:
                strcpy(kv->key, "we");
                break;
            case 1:
                strcpy(kv->key,"love");
                break;
            case 2:
                strcpy(kv->key, "cs");
                break;
            case 3:
                strcpy(kv->key, "3210");
                break;
        }
        kv->val = 1;
    }

    // Busy waiting (to make map task run longer)
    i = 0;
    while (i < INT_MAX) { i++; }

    return output;
}

/* Helper function to free the `MapTaskOutput*` returned by 
 * the various map functions. */
void free_map_task_output(MapTaskOutput *output) {
    free(output->kvs);
    free(output);
    return;
}

/* Reduce function. 
 *
 * `key`: KeyValue.key
 * `vals`: int array (aggregate all KeyValue.val into an array)
 * `len`: length of `vals`
 * */
KeyValue reduce(char key[8], int *vals, int len) {
    int i;
    int sum = 0;
    for (i = 0; i < len; i++) {
        sum += vals[i];
    }

    KeyValue kv;
    strncpy(kv.key, key, 7);
    kv.key[7] = '\0';
    kv.val = sum;
    return kv;
}
