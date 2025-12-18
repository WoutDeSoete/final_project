#include <stdio.h>
#include "datamgr.h"

#include <assert.h>

#include "lib/dplist.h"
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sbuffer.h"


dplist_t *list;

#ifndef SET_MAX_TEMP
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#error SET_MIN_TEMP not set
#endif

#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

typedef uint16_t sensor_id_t, room_id_t;
typedef double sensor_value_t;

//internal data structures
typedef struct sensor_node {
    sensor_id_t sensor_id;
    room_id_t room_id;
    sensor_value_t buffer[RUN_AVG_LENGTH];
    int buf_index;
    int buf_count;
    double sum;
    time_t last_modified;
} sensor_node_t;

// Head of linked list
static sensor_node_t *head = NULL;

// helper functions

//Create a new sensor node
static sensor_node_t *create_node(sensor_id_t sid, uint16_t rid) {
    sensor_node_t *node = calloc(1, sizeof(sensor_node_t));
    node->sensor_id = sid;
    node->room_id = rid;
    node->buf_index = 0;
    node->buf_count = 0;
    node->sum = 0.0;
    node->last_modified = 0;
    return node;
}

static sensor_node_t *find_sensor(uint16_t sensor_id) {
    int length = dpl_size(list);
    for (int i = 0; i < length; i++) {
        sensor_node_t *node = (sensor_node_t *)dpl_get_element_at_index(list, i);
        if (node->sensor_id == sensor_id) return node;
    }
    return NULL;
}

// update buffer with new value
static void update_buffer(sensor_node_t *n, sensor_value_t value, time_t ts) {
    if (n->buf_count < RUN_AVG_LENGTH) {
        n->buffer[n->buf_index] = value;
        n->sum += value;
        n->buf_count++;
    } else {
        /* Circular buffer replace oldest */
        n->sum -= n->buffer[n->buf_index];
        n->buffer[n->buf_index] = value;
        n->sum += value;
    }
    n->buf_index = (n->buf_index + 1) % RUN_AVG_LENGTH;
    n->last_modified = ts;
}

// Compute running average
static double compute_avg(sensor_node_t *n) {
    if (n->buf_count < RUN_AVG_LENGTH)
        return 0.0;
    return n->sum / (double)RUN_AVG_LENGTH;
}


static void *element_copy(void *src_element) {
    ERROR_HANDLER(src_element == NULL, "Null pointer passed to element_copy");
    sensor_node_t *src = (sensor_node_t *)src_element;
    sensor_node_t *dst = malloc(sizeof(sensor_node_t));
    ERROR_HANDLER(dst == NULL, "Memory allocation failed in element_copy");
    memcpy(dst, src, sizeof(sensor_node_t));
    return dst;
}

void element_free(void **element)
{
    free(*element);
    *element = NULL;
}

// Compare callback: compare sensor IDs
static int element_compare(void *x, void *y) {
    sensor_node_t *a = (sensor_node_t *)x;
    sensor_node_t *b = (sensor_node_t *)y;
    return (int)a->sensor_id - (int)b->sensor_id;
}

//Main functions


void datamgr_parse_sensor_files(FILE *fp_sensor_map, sbuffer_t *buf)
{
    list = dpl_create(element_copy, element_free, element_compare);

    ERROR_HANDLER(fp_sensor_map == NULL, "Map file pointer is NULL");


    uint16_t room_id, sensor_id;
    while (fscanf(fp_sensor_map, "%hu %hu", &room_id, &sensor_id) == 2)
    {
        if (find_sensor(sensor_id) != NULL) {
            fprintf(stderr, "Duplicate sensor ID %u found in map file, ignoring\n", sensor_id);
            continue;
        }
        sensor_node_t *node = create_node(sensor_id, room_id);
        dpl_insert_at_index(list, node, 0, true);
        free(node);
    }


    while (1) {
        sensor_data_t data;
        int r = sbuffer_remove(buf, &data);
        if (r != 0|| data.id == 0) {
            // EOS or no more data
            break;
        }


        sensor_node_t *node = find_sensor(data.id);
        if (node == NULL) {
            //TODO: logger message here unknown ID
            continue;
        }

        update_buffer(node, data.value, data.ts);
        double avg = compute_avg(node);
        if (node->buf_count >= RUN_AVG_LENGTH) {
            if (avg > SET_MAX_TEMP)
            {
                //TODO: logger message here too hot
            }


            else if (avg < SET_MIN_TEMP)
            {
                //TODO: logger message here too cold
            }


        }
    }

}

void datamgr_free()
{
    dpl_free(&list, true);
    list = NULL;
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id)
{
    sensor_node_t *node = find_sensor(sensor_id);
    room_id_t room = node->room_id;
    return room;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id)
{
    sensor_node_t *node = find_sensor(sensor_id);
    return compute_avg(node);
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id)
{
    sensor_node_t *node = find_sensor(sensor_id);
    return node->last_modified;
}

int datamgr_get_total_sensors()
{
    return dpl_size(list);
}