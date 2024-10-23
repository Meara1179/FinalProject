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

char* op_names[] = 
{
    "open", "close", "stop", "service_on", "service_off", "up", "down"
};

// Function prototypes.
bool up_operation(car_shared_mem* shm);
bool down_operation(car_shared_mem* shm);
bool open_operation(car_shared_mem* shm);
bool close_operation(car_shared_mem* shm);
bool stop_operation(car_shared_mem* shm);
bool service_on_operation(car_shared_mem* shm);
bool service_off_operation(car_shared_mem* shm);

int main(int argc, char *argv[])
{
    char mem_name[100] = "/car";
    strcat(mem_name, argv[1]);

    int fd = shm_open(mem_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        printf("Unable to access CAR %s", argv[1]);
        return false;
    }

    car_shared_mem*  shared_mem_address = mmap(NULL, sizeof(car_shared_mem), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem_address == MAP_FAILED)
    {
        printf("Mapping failed.");
        return false;
    }

    char* operation = argv[2];

    if (strcmp(operation, op_names[0]) == 0)
    {
        open_operation(shared_mem_address);
    }
    else if (strcmp(operation, op_names[1]) == 0)
    {
        close_operation(shared_mem_address);
    }
    else if (strcmp(operation, op_names[2]) == 0)
    {
        stop_operation(shared_mem_address);
    }
    else if (strcmp(operation, op_names[3]) == 0)
    {
        service_on_operation(shared_mem_address);
    }
    else if (strcmp(operation, op_names[4]) == 0)
    {
        service_off_operation(shared_mem_address);
    }
    else if (strcmp(operation, op_names[5]) == 0)
    {
        //up_operation(shared_mem_address);
    }
    else if (strcmp(operation, op_names[6]) == 0)
    {
        //down_operation(shared_mem_address);
    }
    else
    {
        printf("Invalid operation.\n");
    }

    return true;
}

bool up_operation(car_shared_mem* shm)
{
    if (shm->individual_service_mode == 0)
    {
        printf("Operation only allowed in service mode.\n");
        return false;
    }
    else if (shm->status == "Open" || shm->status == "Opening")
    {
        printf("Operation not allowed while doors are open.\n");
        return false;
    }
    else if (shm->status == "Between")
    {
        printf("Operation not allowed while elevator is moving.\n");
        return false;
    }

    char next_floor[4];
    char* cur_floor_ptr = &shm->current_floor[0];
    uint16_t cur_floor;

    pthread_mutex_lock(&shm->mutex);
    if (shm->current_floor[0] == 'B')
    {
        scanf(shm->current_floor, "%d", &cur_floor);
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
        scanf(shm->current_floor, "%d", &cur_floor);
        sprintf(next_floor, "%d", cur_floor + 1);
    }
    *shm->destination_floor = *next_floor;
    pthread_cond_signal(&shm->cond);
    pthread_mutex_unlock(&shm->mutex);

    return true;
}

bool down_operation(car_shared_mem* shm)
{
    if (shm->individual_service_mode == 0)
    {
        printf("Operation only allowed in service mode.\n");
        return false;
    }
        else if (shm->status == "Open" || shm->status == "Opening")
    {
        printf("Operation not allowed while doors are open.\n");
        return false;
    }
    else if (shm->status == "Between")
    {
        printf("Operation not allowed while elevator is moving.\n");
        return false;
    }

    char next_floor[4];
    char* cur_floor_ptr = &shm->current_floor[0];
    uint16_t cur_floor;

    pthread_mutex_lock(&shm->mutex);
    if (shm->current_floor[0] == 'B')
    {
        scanf(shm->current_floor, "%d", &cur_floor);
        sprintf(next_floor, "B%d", cur_floor + 1);
    }
    else
    {
        scanf(shm->current_floor, "%d", &cur_floor);
        if (cur_floor == 1)
        {
            sprintf(next_floor, "B%d", cur_floor);
        }
        else
        {
            sprintf(next_floor, "%d", cur_floor - 1);
        }
    }

    *shm->destination_floor = *next_floor;
    pthread_cond_signal(&shm->cond);
    pthread_mutex_unlock(&shm->mutex);

    return true;
}

bool open_operation(car_shared_mem* shm)
{
    pthread_mutex_lock(&shm->mutex);
    shm->open_button = 1;
    pthread_cond_signal(&shm->cond);
    pthread_mutex_unlock(&shm->mutex);
    
    return true;
}

bool close_operation(car_shared_mem* shm)
{
    pthread_mutex_lock(&shm->mutex);
    shm->close_button = 1;
    pthread_cond_signal(&shm->cond);
    pthread_mutex_unlock(&shm->mutex);
    
    return true;
}

bool stop_operation(car_shared_mem* shm)
{
    pthread_mutex_lock(&shm->mutex);
    shm->emergency_stop = 1;
    pthread_cond_signal(&shm->cond);
    pthread_mutex_unlock(&shm->mutex);
    
    return true;
}

bool service_on_operation(car_shared_mem* shm)
{
    pthread_mutex_lock(&shm->mutex);
    shm->individual_service_mode = 1;
    shm->emergency_mode = 0;
    pthread_cond_signal(&shm->cond);
    pthread_mutex_unlock(&shm->mutex);
    
    return true;
}

bool service_off_operation(car_shared_mem* shm)
{
    pthread_mutex_lock(&shm->mutex);
    shm->individual_service_mode = 0;
    pthread_cond_signal(&shm->cond);
    pthread_mutex_unlock(&shm->mutex);
    
    return true;
}