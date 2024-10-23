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

char* name;
char* lowest_floor;
char* highest_floor;
uint16_t delay;
car_shared_mem shared_mem;
car_shared_mem* shared_mem_address;

bool shared_mem_init(car_shared_mem* shm, char* mem_name)
{
  strcpy(shm->current_floor, lowest_floor);
  strcpy(shm->destination_floor, lowest_floor);
  strcpy(shm->status, "Closed");
  shm->open_button = 0;
  shm->close_button = 0;
  shm->door_obstruction = 0;
  shm->overload = 0;
  shm->emergency_stop = 0;
  shm->individual_service_mode = 0;
  shm->emergency_mode = 0;

  int fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (fd = -1)
  {
    return false;
  }

  size_t shared_mem_size = sizeof(car_shared_mem);
  int trunc_status = ftruncate(fd, sizeof(shared_mem_size));
  if (trunc_status != 0)
  {
      return false;
  }

  shared_mem_address = mmap(NULL, shared_mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (shared_mem_address == MAP_FAILED)
  {
    return false;
  }

  pthread_mutex_init(&shm->mutex, pthread_mutexattr_setpshared);
  pthread_cond_init(&shm->cond, pthread_condattr_setpshared);

  return true;
}


int car_create(char* name_par, char* lowest_floor_par, char* highest_floor_par, uint16_t delay_par)
{
  name = &name_par;
  lowest_floor = &lowest_floor_par;
  highest_floor = &highest_floor_par;
  delay = delay_par;

  char mem_name[100] = "car";
  strcat(mem_name, name);

  bool shared_mem_status = shared_mem_init(&shared_mem, mem_name);
}