#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <stdbool.h>
#include "sbuffer.h"


//this function writes sensor data gotten from the sbuffer to a csv file.
// This is a reader thread for the sbuffer, and this ends when the writer for the buffer gives the signal
// \param buf this is the shared buffer that this function reads from
void process_data(sbuffer_t *buf);

#endif /* _SENSOR_DB_H_ */