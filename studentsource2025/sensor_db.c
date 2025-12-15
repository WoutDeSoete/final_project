#include "sensor_db.h"

#include <inttypes.h>

#include "config.h"

void process_data(sbuffer_t *buf)
{
    FILE *f = fopen("data.csv", "w");
    //TODO: logger message file created
    if (f == NULL)return;
    while (1) {
        sensor_data_t data;
        int r = sbuffer_remove(buf, &data);
        if (r != 0|| data.id == 0) {
            // EOS or no more data
            break;
        }
        fprintf(f, "%" PRIu32 ",%.6f,%" PRIu64 "\n",
                data.id, data.value, data.ts);
        fflush(f);
        //TODO: logger message sensor data inserted
    }
    fclose(f);
    //TODO: logger message data file closed
}

int close_db(FILE * f)
{
    return fclose(f);
}