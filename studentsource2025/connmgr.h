//
// Created by wout on 12/12/25.
//

#ifndef STUDENTSOURCE2025_CONNMGR_H
#define STUDENTSOURCE2025_CONNMGR_H
#include "lib/tcpsock.h"

#endif //STUDENTSOURCE2025_CONNMGR_H

//function for the creation of the connection threads when a tcp connection comes in
// this function only stops when all the processing threads are done doing their thing
// \param MAX_CONN estabishes the maximum amount of sensors can be handled at the same time
// \param PORT every sensor connection needs an adress to find and that is this port number
int connection_manager(int MAX_CONN, int PORT, sbuffer_t *buffer);

//function for the data processing of 1 sensor, handling all data coming in
// this function stops when the sensor is suspended or when the sensor waits too long to send data
// \param arg contains all the parameters that are given with the thread creation
void *processing_data(void *arg);
