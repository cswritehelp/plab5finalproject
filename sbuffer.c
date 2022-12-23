#include <stdlib.h>
#include <stdio.h>
#include "sbuffer.h"
#include <pthread.h>

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 * notice that this buffer use single pointer which is different with previous labs
 * the reason we use single pointer to link the elements->easy to free
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
    pthread_mutex_t head_mutex; 
    pthread_mutex_t tail_mutex; 
    /*a condition variable allows any number of threads to wait for another thread to signal a condition. 
    Regard it as an event when it is signaled all threads waiting for it are woken up
    Condition variables are always used in conjunction with a mutex to control access to them. To wait on a condition variable:
    pthread_cond_wait(&myConVar , &mymutex); */
    pthread_cond_t read;    
};

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    pthread_mutex_init(&((*buffer)->head_mutex), NULL);
    pthread_mutex_init(&((*buffer)->tail_mutex), NULL);
    pthread_cond_init(&((*buffer)->read), NULL);
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
    pthread_mutex_destroy(&((*buffer)->head_mutex));
    pthread_mutex_destroy(&((*buffer)->tail_mutex));
    pthread_cond_destroy(&((*buffer)->read));
    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data, bool isData) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;
    if (buffer->head == NULL) return SBUFFER_NO_DATA;
    *data = buffer->head->data;
    if(isData) data->data_flag = true;
    else data->storage_flag = true;
    pthread_mutex_lock(&(buffer)->head_mutex);
    if(data->data_flag == true && data->storage_flag == true){
        dummy = buffer->head;
        while (buffer->head == NULL) pthread_cond_wait(&((buffer)->read), &(buffer)->head_mutex);
        if (buffer->head == buffer->tail) // buffer has only one node
        {
            buffer->head = buffer->tail = NULL;
        } else  // buffer has many nodes empty
        {
            buffer->head = buffer->head->next;
        }
        free(dummy);
    }
    pthread_mutex_unlock(&((buffer)->head_mutex));
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;
    dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL) return SBUFFER_FAILURE;
    dummy->data = *data;
    dummy->next = NULL;
    pthread_mutex_lock(&(buffer)->tail_mutex);
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL)
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
    //unlock all the waiting threads
    pthread_cond_broadcast(&((buffer)->read));
    pthread_mutex_unlock(&(buffer->tail_mutex));
    return SBUFFER_SUCCESS;
}
