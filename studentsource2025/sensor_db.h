#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <stdbool.h>
#include "sbuffer.h"


void process_data(sbuffer_t *buf);
int close_db(FILE * f);

#endif /* _SENSOR_DB_H_ */