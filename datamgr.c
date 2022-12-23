#include <inttypes.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "sbuffer.h"
#include "datamgr.h"
#include "./lib/dplist.h"
#include "sensor_db.h"

#define RUN_AVG_LENGTH 5
#define SET_MAX_TEMP 20
#define SET_MIN_TEMP 10

dplist_t *sensor_list;

char data_log_event[MAX_LENGTH];

void * datamgr_element_copy(void *element) {
    sensor_list_element_t* copy = malloc(sizeof(sensor_list_element_t));
    assert(copy != NULL);
    copy->sensor_id = ((sensor_list_element_t*)element)->sensor_id;
    copy->room_id = ((sensor_list_element_t*)element)->room_id;
    copy->last_modified = ((sensor_list_element_t*)element)->last_modified;
    memcpy(copy->temperature, ((sensor_list_element_t*)element)->temperature, sizeof(copy->temperature));
    copy->index = ((sensor_list_element_t*)element)->index;
    copy->size = ((sensor_list_element_t*)element)->size;
    return copy;
}

void datamgr_element_free(void **element) {
    free(*element);
    *element = NULL;
}

int datamgr_element_compare(void *x, void *y) {
    return ((((sensor_list_element_t*)x)->sensor_id < ((sensor_list_element_t*)y)->sensor_id) ? -1 : (((sensor_list_element_t*)x)->sensor_id == ((sensor_list_element_t*)y)->sensor_id) ? 0 : 1);
}

void datamgr_parse_sensor_data(FILE* fp_sensor_map, sbuffer_t *sbuffer, int pipefd){
    room_id_t room_id;
    sensor_id_t sensor_id;
    sensor_list = dpl_create(datamgr_element_copy, datamgr_element_free, datamgr_element_compare);
    sensor_list_element_t* sensor_list_element = malloc(sizeof(sensor_list_element_t));
    sensor_list_element->index = 0;
    sensor_list_element->size = 0;
    memset(sensor_list_element->temperature, 0, sizeof(sensor_list_element->temperature));
    while(fscanf(fp_sensor_map,"%hu %hu\n", &room_id, &sensor_id) != EOF){
            sensor_list_element->room_id = room_id;
            sensor_list_element->sensor_id = sensor_id;
            dpl_insert_at_index(sensor_list, sensor_list_element, dpl_size(sensor_list), true);
        }
    free(sensor_list_element);
    sensor_list_element = NULL;
    fclose(fp_sensor_map);

    while(1)
    {
        sensor_data_t* sensor_data;
        sensor_data = malloc(sizeof(sensor_data_t));
        //It is important to break from the loop at some point, otherwise it will become an infinite loop
        if(sbuffer_remove(sbuffer, sensor_data, true) == SBUFFER_NO_DATA) break;
        //check whether the sensor id is valid
        sensor_list_element_t* dummy = datamgr_get_sensor_element(sensor_data->id);
        //if it is invalid
        if(dummy == NULL){
            sprintf(data_log_event, "Received sensor data with invalid sensor node ID %" PRIu16 "\n", dummy->sensor_id);
            write_log_event(data_log_event, pipefd);
        }
        dummy->last_modified = sensor_data->ts;
        if(dummy->size < RUN_AVG_LENGTH) dummy->size++;
        dummy->temperature[dummy->index] = sensor_data->value;
        dummy->index++;
        if(dummy->index >= RUN_AVG_LENGTH) dummy->index = 0;

        sensor_value_t running_avg = datamgr_get_avg(dummy->sensor_id);
        if(running_avg > SET_MAX_TEMP){
            sprintf(data_log_event, "Sensor node %" PRIu16" reports it's too hot (avg temp = %lf)\n", dummy->sensor_id, running_avg);
            write_log_event(data_log_event, pipefd);
        }
        if(running_avg < SET_MIN_TEMP){
            sprintf(data_log_event, "Sensor node %" PRIu16" reports it's too cold (avg temp = %lf)\n", dummy->sensor_id, running_avg);
            write_log_event(data_log_event, pipefd);
        }
    }
}

void datamgr_free(){
    dpl_free(&sensor_list, true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id){
    return datamgr_get_sensor_element(sensor_id)->room_id;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id){
    return datamgr_get_sensor_element(sensor_id)->last_modified;
}

sensor_list_element_t *datamgr_get_sensor_element(sensor_id_t sensor_id){
    for(int i = 0; i < dpl_size(sensor_list); i++){
        sensor_list_element_t* sensor_list_element;
        //typecast void* to sensor_list_element_t*
        sensor_list_element = (sensor_list_element_t*)dpl_get_element_at_index(sensor_list, i);
        if(sensor_list_element->sensor_id == sensor_id) return sensor_list_element;
    }
    return NULL;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id){
    sensor_value_t running_sum = 0;
    sensor_value_t running_avg = 0;
    sensor_list_element_t *sensor_list_element;
    sensor_list_element = datamgr_get_sensor_element(sensor_id);

    for(int i = 0; i < sensor_list_element->size; i++){
        running_sum += sensor_list_element->temperature[i];
        running_avg = running_sum/sensor_list_element->size;
    }
    return running_avg;
}

int datamgr_get_total_sensors(){
    return dpl_size(sensor_list);
}
