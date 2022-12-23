#ifndef CONNMGR_H_
#define CONNMGR_H_

#include <pthread.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include "sbuffer.h"
#include "config.h"
#include "lib/tcpsock.h"
#include "lib/dplist.h"

#ifndef TIMEOUT
#error TIMEOUT not set
#endif

typedef struct {
    pthread_t* conn_pthread;
}connection_list_element_t;

void* connmgr_element_copy(void* elemenr);
void connmgr_element_free(void** element);
int connmgr_element_compare(void *x, void *y);

void connmgr_free();

void connmgr_server(int PORT, sbuffer_t* sbuffer, int fd);

void* func(void* param);

#endif  //CONNMGR_H_