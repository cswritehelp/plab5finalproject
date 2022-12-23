#include "config.h"
#include "connmgr.h"
#include "datamgr.h"
#include "sbuffer.h"
#include "sensor_db.h"
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE 100

struct thread_data
{
    int PORT;
    int fd;
};

void *connmgr(void *param);
void *datamgr(void *param);
void *storagemgr(void *param);

sbuffer_t *sbuffer;
int pipefd[2];

int main(int argc, char* argv[]){
    pid_t pid;
    int rc;
    if(argc < 2){
        printf("No extra command line argument passed other than program name.");
        return 1;
    }
    int PORT = atoi(argv[1]);
    printf("port number = %d\n", PORT);
    pthread_t connmgr_thread, datamgr_thread, storagemgr_thread;
    if(sbuffer_init(&sbuffer) != SBUFFER_SUCCESS) exit(SBUFFER_FAILURE);
    if (pipe(pipefd) == -1){
        perror("pipe initialization failed");
        exit(EXIT_FAILURE);
    }
    struct thread_data td;
    td.PORT = PORT;
    td.fd = pipefd[WRITE_END];
    pid = fork();
    if(pid < 0){
        perror("fork initialization failed");
        exit(EXIT_FAILURE);
    }
    //parent process
    else if(pid > 0){
        close(pipefd[READ_END]);
        //On success, pthread_create() returns 0; on error, it returns an error number, and the contents of *thread are undefined.
        rc = pthread_create(&connmgr_thread, NULL, connmgr, (void*) &td);    
        if(rc){
            printf("Error creating thread; return code from pthread_create() is %d\n", rc);
            exit(EXIT_FAILURE);
        }
        rc = pthread_create(&datamgr_thread, NULL, datamgr, &pipefd[WRITE_END]);
        if(rc){
             printf("Error creating thread; return code from pthread_create() is %d\n", rc);
             exit(EXIT_FAILURE);
        }
         rc = pthread_create(&storagemgr_thread, NULL, storagemgr, &pipefd[WRITE_END]);
         if(rc){
             printf("Error creating thread; return code from pthread_create() is %d\n", rc);
             exit(EXIT_FAILURE);
        }
        pthread_join(connmgr_thread, NULL);
        pthread_join(datamgr_thread, NULL);
        pthread_join(storagemgr_thread, NULL);
        close(pipefd[WRITE_END]);
        if(sbuffer_free(&sbuffer) != SBUFFER_SUCCESS) exit(SBUFFER_FAILURE);
    }
    //child process 
    else{
        int sequence_number = 0;
        FILE* fp_gateway;
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        fp_gateway = fopen("gateway.log", "w");
        close(pipefd[WRITE_END]);
        while((bytes_read = read(pipefd[READ_END], buffer, MAX_LENGTH)) > 0){
            sequence_number++;
            fprintf(fp_gateway, "%2d %ld %s", sequence_number, time(NULL), buffer);
            fflush(fp_gateway);
        } 
        close(pipefd[READ_END]);
        fclose(fp_gateway);
        exit(EXIT_SUCCESS);
    }
    return 0;
}

void *connmgr(void* param){
    struct thread_data *my_data;
    my_data = (struct thread_data*) param;
    int PORT = my_data->PORT;
    int fd = my_data->fd;
    connmgr_server(PORT, sbuffer, fd);
    connmgr_free();
    pthread_exit(NULL);
}

void *datamgr(void *param){
    int fd = *((int*)param);
    FILE* fp;
    fp = fopen("room_sensor.map", "r");
    if(fp == NULL) fprintf(stderr, "No such file");
    datamgr_parse_sensor_data(fp, sbuffer, fd);
    datamgr_free();
    pthread_exit(NULL);
}

void *storagemgr(void *param){
    int fd = *((int*)param);
    storage_manager(sbuffer, fd);
    pthread_exit(NULL);
}

void write_log_event(char* log_event, int fd){
    pthread_mutex_t pipe_mutex;
    pthread_mutex_init(&pipe_mutex, NULL);
    pthread_mutex_lock(&pipe_mutex);
    write(fd, log_event, MAX_LENGTH);
    pthread_mutex_unlock(&pipe_mutex);
    pthread_mutex_destroy(&pipe_mutex);
}