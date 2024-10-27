#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "car_mem_struct.h"

// Function prototypes
int verify_data(car_shared_mem *shm);
int verify_floors(char *cur_floor, char *dest_floor);
int verify_status(char status[8]);
int verify_fields(car_shared_mem *shm);
int verify_door_obstruct(car_shared_mem *shm);

int main(int argc, char *argv[])
{
    assert(argc == 2);
    assert(strlen(argv[1]) <= 95);

    char mem_name[100] = "/car";
    strcat(mem_name, argv[1]);

    int fd = shm_open(mem_name, O_RDWR, 438);
    if (fd == -1)
    {
        (void)printf("Unable to access car %s\n", argv[1]);
    }
    else
    {
        car_shared_mem*  shm = mmap(NULL, sizeof(car_shared_mem), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (shm == MAP_FAILED)
        {
            (void)printf("Mapping failed.\n");
        }
        else
        {
            pthread_mutex_lock(&shm->mutex);
            for (;;)
            {
                (void)pthread_cond_wait(&shm->cond, &shm->mutex);
                if (shm->emergency_stop == 1 && shm->emergency_mode == 0)
                {
                    (void)printf("The emergency stop button has been pressed!\n");
                    shm->emergency_mode = 1;
                }
                if (shm->overload == 1 && shm->emergency_mode == 0)
                {
                    (void)printf("The overload sensor has been tripped!\n");
                    shm->emergency_mode = 1;
                }
                else if (verify_data(shm) == 0)
                {
                    (void)printf("Data consistency error!\n");
                    shm->emergency_mode = 1;
                }
            }
        }
    }


    return 1;
}

int verify_data(car_shared_mem *shm)
{
    int valid;

    if (verify_floors(shm->current_floor, shm->destination_floor) == 0)
    {
        valid = 0;
    }
    else if (verify_status(shm->status) == 0)
    {
        valid = 0;
    }
    else if (verify_fields(shm) == 0)
    {
        valid = 0;
    }
    else if (verify_door_obstruct(shm) == 0)
    {
        valid = 0;
    }
    else
    {
        valid = 1;
    }

    return valid;
}

int verify_floors(char *cur_floor, char *dest_floor)
{
    assert(cur_floor != NULL || dest_floor != NULL);
    int valid;

    if (strlen(cur_floor) > 3 || strlen(dest_floor) > 3)
    {
        valid = 0;
    }
    else if (cur_floor[0] != 'B' && isdigit(cur_floor[0]) == 0)
    {
        valid = 0;
    }
    else if (dest_floor[0] != 'B' && isdigit(dest_floor[0]) == 0)
    {
        valid = 0;
    }
    else
    {
        valid = 1;
    }

    assert(valid == 0 || valid == 1);
    return valid;
}

int verify_status(char status[8])
{
    assert(status != NULL);
    assert(strlen(status) <= 8);
    int valid = 0;
    const char* valid_status[] = 
    {
        "Opening", "Open", "Closing", "Closed", "Between"
    };

    for (int i = 0; i < 5; i++)
    {
        if (strcmp(status, valid_status[i]) == 0)
        {
            valid = 1;
        }
    }

    assert(valid == 0 || valid == 1);
    return valid;
}

int verify_fields(car_shared_mem *shm)
{
    assert(shm != NULL);
    int valid;

    if (shm->open_button > 1)
    {
        valid = 0;
    }
    else if (shm->close_button > 1)
    {
        valid = 0;
    }
    else if (shm->door_obstruction > 1)
    {
        valid = 0;
    }
    else if (shm->overload > 1)
    {
        valid = 0;
    }
    else if (shm->emergency_stop > 1)
    {
        valid = 0;
    }
    else if (shm->individual_service_mode > 1)
    {
        valid = 0;
    }
    else if (shm->emergency_mode > 1)
    {
        valid = 0;
    }
    else
    {
        valid = 1;
    }
    assert(valid == 0 || valid == 1);
    return valid;
}

int verify_door_obstruct(car_shared_mem *shm)
{
    assert(shm != NULL);
    int valid = 0;

    if (shm->door_obstruction == 1)
    {
        if (strcmp(shm->status, "Opening") == 0 || strcmp(shm->status, "Closing") == 0)
        {
            valid = 1;
        }
        else
        {
            valid = 0;
        }
    }
    else
    {
        valid = 1;
    } 

    assert(valid == 0 || valid == 1);
    return valid;
}