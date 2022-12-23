#ifndef _CONFIG_H_
#define _CONFIG_H_
//MAX_LENGTH is defined in config.h and only define once, otherwise give a warning about redefine
#define MAX_LENGTH 100

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>

typedef uint16_t sensor_id_t;
typedef uint16_t room_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
    bool data_flag;                  /**< a bool that indicates whether the sbuffer node has been analyzed by the datamgr*/
    bool storage_flag;               /**< a bool that indicates whether the sbuffer node has been analyzed by the storagemgr*/
} sensor_data_t;

void write_log_event(char* log_event, int fd);

#endif /* _CONFIG_H_ */
