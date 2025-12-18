//
// Created by wout on 12/12/25.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "sbuffer.h"

#define READ_END 0
#define WRITE_END 1

int logger_loop();
int write_to_logger(char *msg);

int fd[2];
pid_t pid;

int main(int argc, char* argv[])
{

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
        logger_loop();
    }
    close(fd[READ_END]); // parent doesn't read

    //setting up shared buffer
    sbuffer_t *buf;
    sbuffer_init(&buf);
    if (!buf) {
        write_to_logger( "Failed to init sbuffer\n");
        return -1;
    }
    write_to_logger("buffer created\n");

    write_to_logger("END\n");

}

int logger_loop()
{
    printf("logger loop entered\n");
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

            printf("child: logging message: %s\n", msg_buffer);

            time(&currentTime);
            char *time = ctime(&currentTime);
            time[strcspn(time, "\n")] = '\0';
            fprintf(logfile, "%d - %s - %s", count++, time, msg_buffer);
            fflush(logfile);
        }

    }


    fclose(logfile);
    fclose(log_stream);
    exit(0);
}

int write_to_logger(char *msg)
{
    if (pid <= 0) return -1;
    if (!msg) return -1;
    printf("[LOG WRITE] %s", msg);

    write(fd[WRITE_END], msg, strlen(msg));
    return 0;
}
