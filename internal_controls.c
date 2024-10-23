#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h> 
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "car_mem_struct.h"

int internal_controls_create(char* name_par, char* operation)
{
    char mem_name[100] = "car";
    strcat(mem_name, name_par);

    int fd = shm_open(mem_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd = -1)
    {
        printf("Unable to access car %s", name_par);
        return false;
    }

    car_shared_mem*  shared_mem_address = mmap(NULL, sizeof(car_shared_mem), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem_address == MAP_FAILED)
    {
        return false;
    }

    return true;
}


bool up_operation(car_shared_mem* shm)
{
    if (shm->individual_service_mode == 0)
    {
        printf("Operation only allowed in service mode.");
        return false;
    }

    char next_floor[4];
    uint16_t cur_floor;

    if (shm->current_floor[0] == "B")
    {
        cur_floor = atoi(shm->current_floor[1]);
        if (cur_floor == 1)
        {
            sprintf(next_floor, "%d", cur_floor);
        }
        else
        {
            sprintf(next_floor, "B%d", cur_floor - 1);
        }
    }
    else
    {
        cur_floor = atoi(shm->current_floor);
        sprintf(next_floor, "%d", cur_floor + 1);
    }
    *shm->destination_floor = next_floor;
}