#include "connmgr.h"
#include "sensor_db.h"
#include <pthread.h>

#define TIMEOUT 5
#define MAX_CONN 3

struct thread_params
{
    tcpsock_t* thread_client;
    pthread_t* thread_pthread;
};

dplist_t* connection_list = NULL;
char conn_log_event[MAX_LENGTH];
int conn_fd;
sbuffer_t* buffer;
int conn_counter = 0;
bool flag = true;

void *func(void *param);

void* connmgr_element_copy(void* element){
    connection_list_element_t* copy = malloc(sizeof(connection_list_element_t));
    copy->conn_pthread = ((connection_list_element_t*)element)->conn_pthread;
    return copy;
}

void connmgr_element_free(void **element){
    free(*element);
    *element = NULL;
}

int connmgr_element_compare(void *x, void* y){
    return ((((connection_list_element_t*)x)->conn_pthread < ((connection_list_element_t*)y)->conn_pthread) ? -1 : (((connection_list_element_t*)x)->conn_pthread == ((connection_list_element_t*)y)->conn_pthread) ? 0 : 1);
}

void connmgr_server(int PORT, sbuffer_t* sbuffer, int fd){
    buffer = sbuffer;
    conn_fd = fd;
    tcpsock_t *server;
    printf("Test server is started.\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    connection_list = dpl_create(connmgr_element_copy, connmgr_element_free, connmgr_element_compare);
    do
    {
        pthread_t* pthread = malloc(sizeof(pthread_t));
        struct thread_params tp;
        tp.thread_pthread = pthread;
        //handle incoming connection
        if(tcp_wait_for_connection(server, &tp.thread_client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        printf("Incoming client connection.\n");
        connection_list_element_t* connection_list_element;
        connection_list_element = malloc(sizeof(connection_list_element_t));
        connection_list_element->conn_pthread = pthread;
        connection_list = dpl_insert_at_index(connection_list, connection_list_element, dpl_size(connection_list), false);
        conn_counter++;
        pthread_create(pthread, NULL, func, &tp);
    }while (conn_counter < MAX_CONN);
    flag = false;
    for(int i = 0; i < dpl_size(connection_list); i++){
        connection_list_element_t* connection_list_element = (connection_list_element_t*) dpl_get_element_at_index(connection_list, i);
        pthread_join(*(connection_list_element->conn_pthread), NULL);
    }
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down.\n");
    sensor_data_t* sensor_data = malloc(sizeof(sensor_data_t));
    sensor_data->id = 0;
    sensor_data->ts = 0;
    sensor_data->value = 0;
    if(sbuffer_insert(buffer, sensor_data) != SBUFFER_SUCCESS) exit(SBUFFER_FAILURE);
    free(sensor_data);
}

//handle incoming data
void *func(void *param){
    struct thread_params* my_params = (struct thread_params*) param;
    tcpsock_t* client = my_params->thread_client;
    pthread_t* pthread = my_params->thread_pthread;
    sensor_data_t data;
    int result, bytes;
    int counter = 0;
    do {
            // read sensor ID
            counter++;
            bytes = sizeof(data.id);
            result = tcp_receive(client, (void *) &data.id, &bytes);
            if(counter == 1){
                sprintf(conn_log_event, "Sensor node %"PRIu16" has opened a new connection.\n", data.id);
                write_log_event(conn_log_event, conn_fd);
            }
            // read temperature
            bytes = sizeof(data.value);
            result = tcp_receive(client, (void *) &data.value, &bytes);
            // read timestamp
            bytes = sizeof(data.ts);
            result = tcp_receive(client, (void *) &data.ts, &bytes);
            //insert into sbuffer
            data.data_flag = false;
            data.storage_flag = false;
            if(sbuffer_insert(buffer, &data) != SBUFFER_SUCCESS) exit(EXIT_FAILURE);
            sprintf(conn_log_event, "Sensor node %" PRIu16 " has inserted a data into sbuffer.\n", data.id);
            write_log_event(conn_log_event, conn_fd);
            if ((result == TCP_NO_ERROR) && bytes) {
                printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                       (long int) data.ts);
            }
        } while (result == TCP_NO_ERROR);
        if (result == TCP_CONNECTION_CLOSED){
            printf("Peer has closed connection.\n");
            if(flag) dpl_remove_at_index(connection_list, dpl_get_index_of_element(connection_list, pthread), false);
        } 
        else printf("Error occured on connection to peer.\n");
        tcp_close(&client);
        sprintf(conn_log_event, "Sensor node %" PRIu16 " has closed the connection.\n", data.id);
        write_log_event(conn_log_event, conn_fd);
    return NULL;
}

void connmgr_free(){
    dpl_free(&connection_list, false);
}
