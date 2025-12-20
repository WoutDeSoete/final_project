#include "sensor_db.h"

#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

void process_data(sbuffer_t *buf)
{
    FILE *f = fopen("data.csv", "w");
    char *msg1 = "a new data.csv file has been created\n";
    pthread_mutex_lock(&pipe_mutex);
    write(pipe_write_fd, msg1, strlen(msg1));
    pthread_mutex_unlock(&pipe_mutex);

    if (f == NULL)return;
    while (1) {
        sensor_data_t data;
        int r = sbuffer_remove(buf, &data);
        if (r != 0|| data.id == 0) {
            // EOS or no more data
            break;
        }
        fprintf(f, "%" PRIu32 ",%.6f,%" PRIu64 "\n",
                data.id, data.value, data.ts);
        fflush(f);
        char msg2[50];
        sprintf(msg2, "data insertion from sensor node %d succeeded\n", data.id);
        pthread_mutex_lock(&pipe_mutex);
        write(pipe_write_fd, msg2, strlen(msg2));
        pthread_mutex_unlock(&pipe_mutex);
    }
    fclose(f);
    char *msg3 = "the data.csv file has been closed\n";
    pthread_mutex_lock(&pipe_mutex);
    write(pipe_write_fd, msg3, strlen(msg3));
    pthread_mutex_unlock(&pipe_mutex);
}