/**
 * \author {AUTHOR}
 */

#include <stdlib.h>
#include "sbuffer.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dataavailable = PTHREAD_COND_INITIALIZER;
/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
    bool done;

};

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    (*buffer)->done = false;
    //logging message
    char *msg = "buffer created\n";
    pthread_mutex_lock(&pipe_mutex);
    write(pipe_write_fd, msg, strlen(msg));
    pthread_mutex_unlock(&pipe_mutex);
    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    sbuffer_node_t *dummy;
    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }
    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    //TODO: condition variable behavior for the shared buffer
    if (buffer == NULL) return SBUFFER_FAILURE;
    pthread_mutex_lock(&file_lock);
    if (buffer->head == NULL)
    {
        while (buffer->head == NULL && !buffer->done) pthread_cond_wait(&dataavailable, &file_lock);
        if (buffer->done)
        {
            pthread_cond_signal(&dataavailable); //wake up other reading threads if any
            pthread_mutex_unlock(&file_lock);
            return SBUFFER_NO_DATA;
        }
    }//setting data and removing node happens here (see original code)
    *data = buffer->head->data;
    dummy = buffer->head;
    if (buffer->head == buffer->tail) // buffer has only one node
    {
        buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
        buffer->head = buffer->head->next;
    }
    if (data->id == 0)
    {
        buffer->done = true;
        pthread_cond_signal(&dataavailable);//other threads might be waiting ->next !
        pthread_mutex_unlock(&file_lock);
        return SBUFFER_NO_DATA;
    }
    pthread_mutex_unlock(&file_lock);
    free(dummy);
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    if (buffer == NULL || data == NULL) return SBUFFER_FAILURE;
    dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL) return SBUFFER_FAILURE;
    dummy->data = *data;
    dummy->next = NULL;
    pthread_mutex_lock(&file_lock);
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
    pthread_mutex_unlock(&file_lock);
    pthread_cond_signal(&dataavailable);
    return SBUFFER_SUCCESS;
}