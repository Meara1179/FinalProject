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

#define MILLISECOND 1000;

char* name;
char* lowest_floor;
char* highest_floor;
int delay;

// Function prototypes
bool shared_mem_init(car_shared_mem* shm, char* mem_name);
int cond_handler(car_shared_mem *shm);
void *process_doors(void *p);

int main(int argc, char *argv[])
{
  name = argv[1];
  lowest_floor = argv[2];
  highest_floor = argv[3];
  sscanf(argv[4], "%d", &delay);
  delay = delay * MILLISECOND;

  char mem_name[100] = "car";
  strcat(mem_name, name);

  shm_unlink(mem_name);

  int fd = shm_open(mem_name, O_CREAT | O_RDWR, 0666);
  if (fd == -1)
  {
    printf("shm_open failed.");
    return false;
  }

  int trunc_status = ftruncate(fd, sizeof(car_shared_mem));
  if (trunc_status != 0)
  {
    printf("ftruuncate failed.");
    return false;
  }

  car_shared_mem *shm = mmap(NULL, sizeof(*shm), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (shm == MAP_FAILED)
  {
    printf("mmap failed.");
    return false;
  }

  shared_mem_init(shm, mem_name);

  pthread_t tid;
  pthread_create(&tid, NULL, process_doors, shm);
  pthread_join(tid, NULL);

  return 1;
}

bool shared_mem_init(car_shared_mem *shm, char* mem_name)
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

  pthread_mutexattr_t mutexattr;
  pthread_mutexattr_init(&mutexattr);
  pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);

  pthread_condattr_t condattr;
  pthread_condattr_init(&condattr);
  pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);

  pthread_mutex_init(&shm->mutex, &mutexattr);
  pthread_cond_init(&shm->cond, &condattr);

  pthread_mutexattr_destroy(&mutexattr);
  pthread_condattr_destroy(&condattr);

  return true;
}

void* process_doors(void *p)
{
  car_shared_mem *shm = p;
  //pthread_mutex_lock(&shm->mutex);
  for (;;)
  {
    pthread_cond_wait(&shm->cond, &shm->mutex);
    if (shm->open_button == 1)
    {
      strcpy(shm->status, "Open");
      shm->open_button = 0;
      printf("Hi\n");
    }
    else if (shm->close_button == 1)
    {
      strcpy(shm->status, "Closed");
      shm->close_button = 0;
      printf("Hi\n");
    }
    pthread_mutex_unlock(&shm->mutex);
  }
}
