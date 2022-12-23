#ifndef SENSOR_DB_H_
#define SENSOR_DB_H_

#include "config.h"
#include "sbuffer.h"
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>

/**
 * This function holds the core functionality of storage manager
 * \param sbuffer make sure all the threads use the same sbuffer
 * \param pipefd the file descriptor used for writing operation
*/
void storage_manager(sbuffer_t* sbuffer, int pipefd);

#endif


