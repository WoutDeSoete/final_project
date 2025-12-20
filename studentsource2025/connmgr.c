/**
 * \author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "sbuffer.h"
#include "lib/tcpsock.h"

/**
 * Implements a sequential test server (only one connection at the same time)
 */

typedef struct
{
    tcpsock_t *client;
    sbuffer_t *buf;
}processor_arg_t;



void *processing_data(void *arg);

int connection_manager(int MAX_CONN, int PORT, sbuffer_t *buffer) {
    tcpsock_t *server;
    tcpsock_t *clients[MAX_CONN];
    int conn_counter = 0;

    pthread_t tid[MAX_CONN];

    processor_arg_t arg = {.buf = buffer};
    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    do {
        tcpsock_t *client = clients[conn_counter];
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);

        arg.client = client;
        pthread_create(&tid[conn_counter], NULL, processing_data, &arg);
        conn_counter++;

    } while (conn_counter < MAX_CONN);

    for (int i = 0; i < conn_counter; i++)
    {
        pthread_join(tid[i], NULL);
    }
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Connection manager is shutting down\n");
    //signaling to readers end of stream
    sensor_data_t eos = {0, 0.0, 0};
    sbuffer_insert(buffer, &eos);
    return 0;
}

void *processing_data(void *arg)
{
    processor_arg_t *process = arg;
    tcpsock_t  *client = process->client;
    sbuffer_t *buffer = process->buf;
    sensor_data_t data;
    int bytes, result;
    bool first_conn = true;

    do {
            // read sensor ID
            bytes = sizeof(data.id);
            tcp_receive(client, (void *) &data.id, &bytes);
            //TODO: logger message sensor <ID> connected
            //logging message
            if (first_conn)
            {
                printf("sensor node %d has opened a new connection\n", data.id);
                char msg[50];
                sprintf(msg, "sensor node %d has opened a new connection\n", data.id);
                pthread_mutex_lock(&pipe_mutex);
                write(pipe_write_fd, msg, strlen(msg));
                pthread_mutex_unlock(&pipe_mutex);
                first_conn = false;
            }

            bytes = sizeof(data.value);
            result = tcp_receive(client, (void *) &data.value, &bytes);
            // read timestamp
            bytes = sizeof(data.ts);
            result = tcp_receive(client, (void *) &data.ts, &bytes);
            if ((result == TCP_NO_ERROR) && bytes) {
                //printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                //       (long int) data.ts);

                sbuffer_insert(buffer, &data);
            }
    } while (result == TCP_NO_ERROR);
    if (result == TCP_CONNECTION_CLOSED)
    {
        char msg[50];
        sprintf(msg, "sensor node %d has closed the connection\n", data.id);
        pthread_mutex_lock(&pipe_mutex);
        write(pipe_write_fd, msg, strlen(msg));
        pthread_mutex_unlock(&pipe_mutex);
    }


    else
        printf("Error occured on connection to peer\n");
    tcp_close(&client);

    pthread_exit(NULL);

}



