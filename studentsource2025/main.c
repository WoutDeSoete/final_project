//
// Created by wout on 12/12/25.
//
#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "sbuffer.h"
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"

#define READ_END 0
#define WRITE_END 1

typedef struct
{
    int max;
    int port;
    sbuffer_t *buf;

}connmgr_arg_t;

int fd[2];
pid_t pid;

int pipe_write_fd = -1;
pthread_mutex_t pipe_mutex = PTHREAD_MUTEX_INITIALIZER;

void *connection_thread(void *arg);
void *data_thread(void *arg);
void *storage_thread(void *arg);

int logger_loop();
int write_to_logger(char *msg);

int main(int argc, char* argv[])
{
    if(argc < 3) {
        printf("Please provide the right arguments: first the port, then the max number of clients");
        return -1;
    }
    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    // creating the pipe for logger
    if (pipe(fd) == -1){
        printf("Pipe failed\n");
        return -1;
    }
    // fork the child
    pid = fork();
    if (pid < 0){ // fork error
        printf("fork failed\n");
        return -1;
    }
    if (pid == 0)
    {
        // child of the program
        logger_loop();
    }
    close(fd[READ_END]); // parent doesn't read
    pipe_write_fd = fd[WRITE_END];

    //setting up shared buffer
    sbuffer_t *buf;
    sbuffer_init(&buf);
    if (!buf) {
        write_to_logger( "Failed to init sbuffer\n");
        return -1;
    }

    pthread_t connmgr;
    pthread_t datamgr;
    pthread_t storagemgr;

    connmgr_arg_t con_arg = {.max = MAX_CONN, .port = PORT, .buf = buf};

    if (pthread_create(&connmgr, NULL, connection_thread, &con_arg) != 0) {
        perror("pthread_create connmgr");
        write_to_logger("END\n");
        sbuffer_free(&buf);
        return 1;
    }

    if (pthread_create(&datamgr, NULL, data_thread, buf) != 0)
    {
        perror("pthread_create datamgr");
        write_to_logger("END\n");
        sbuffer_free(&buf);
        return 1;
    }

    if (pthread_create(&storagemgr, NULL, storage_thread, buf) != 0)
    {
        perror("pthread_create storagemgr");
        write_to_logger("END\n");
        sbuffer_free(&buf);
        return 1;
    }

    // Wait for threads
    pthread_join(connmgr, NULL);
    pthread_join(datamgr, NULL);
    pthread_join(storagemgr, NULL);

    // cleanup
    sbuffer_free(&buf);
    write_to_logger("END\n");
    close(fd[WRITE_END]);
    wait(NULL);
    pid = -1;
    printf("end of the main file\n");
    return 0;

}

void *connection_thread(void *arg)
{
    connmgr_arg_t *ca = arg;
    int max = ca->max;
    int port = ca->port;
    sbuffer_t *buf = ca->buf;
    connection_manager(max, port, buf);
    return NULL;
}

void *data_thread(void *arg)
{
    sbuffer_t *buf = arg;
    FILE * map = fopen("room_sensor.map", "r");
    datamgr_parse_sensor_files(map, buf);
    datamgr_free();
    fclose(map);
    return NULL;
}

void *storage_thread(void *arg)
{
    sbuffer_t *buf = arg;
    process_data(buf);
    return NULL;
}

int logger_loop()
{
    close(fd[WRITE_END]);  // child does NOT write

    FILE *logfile = fopen("gateway.log", "w");
    if (!logfile) {
        perror("logger: cannot open gateway.log");
        exit(1);
    }

    char *msg_buffer = NULL;
    size_t buf_size = 0;
    time_t currentTime;
    int count = 0;
    ssize_t n;


    FILE *log_stream = fdopen(fd[0], "r");
    if (!log_stream) {
        perror("fdopen");
        exit(1);
    }

    while ((n = getline(&msg_buffer, &buf_size, log_stream)) != -1)
    {
        if (n > 1)
        {
            if (strcmp(msg_buffer, "END\n") == 0) break;

            printf("logging message: %s\n", msg_buffer);

            time(&currentTime);
            char *time = ctime(&currentTime);
            time[strcspn(time, "\n")] = '\0';
            fprintf(logfile, "%d - %s - %s", count++, time, msg_buffer);
            fflush(logfile);
        }

    }

    free(msg_buffer);
    fclose(logfile);
    fclose(log_stream);
    exit(0);
}

int write_to_logger(char *msg)
{
    if (pid <= 0) return -1;
    if (!msg) return -1;

    write(fd[WRITE_END], msg, strlen(msg));
    return 0;
}
