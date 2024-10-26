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
#include <signal.h>

#include "car_mem_struct.h"

#define MILLISECOND 1000;

car_shared_mem *shm;
char mem_name[100];
char* name;
char* lowest_floor;
char* highest_floor;
int delay;

// Function prototypes
int str_to_int(char str[]);
bool shared_mem_init(car_shared_mem* shm, char* mem_name);
void* change_floor(void *p);
void* open_door(void *p);
void* close_door(void *p);
void sigint_handler(int sig);


int main(int argc, char *argv[])
{
  name = argv[1];
  lowest_floor = argv[2];
  highest_floor = argv[3];
  sscanf(argv[4], "%d", &delay);
  delay = delay * MILLISECOND;

  strcpy(mem_name, "car");
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

  signal(SIGINT, sigint_handler);

  pthread_mutex_lock(&shm->mutex);
  while (1)
  {
    pthread_cond_wait(&shm->cond, &shm->mutex);
    if (strcmp(shm->current_floor, shm->destination_floor) != 0)
    {
      pthread_t t_change_floor;
      pthread_create(&t_change_floor, NULL, change_floor, shm);
      pthread_detach(t_change_floor);
    }
    if (shm->open_button == 1)
    {
      shm->open_button = 0;
      pthread_t t_open_door;
      pthread_create(&t_open_door, NULL, open_door, shm);
      pthread_detach(t_open_door);
    }
    else if (shm->close_button == 1)
    {
      shm->close_button = 0;
      pthread_t t_close_door;
      pthread_create(&t_close_door, NULL, close_door, shm);
      pthread_detach(t_close_door);
  }
  }

  return 0;
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

int str_to_int(char str[])
{
  int rtn = 0;

  for (int i = 0; str[i] != '\0'; i++)
  {
    if (str[i] >= 48 && str[i] <= 57)
    {
      rtn = rtn * 10 + (str[i] - 48);
    }
  }

  return rtn;
}

void* change_floor(void *p)
{
  car_shared_mem *shm = p;

  bool cur_is_basement = false;
  bool dest_is_basement = false;
  int cur_int = str_to_int(shm->current_floor);
  int dest_int = str_to_int(shm->destination_floor);

  if (shm->current_floor[0] == 'B')
  {
    cur_is_basement = true;
  }
  if (shm->destination_floor[0] == 'B')
  {
    dest_is_basement = true;
  }

  char cur_floor[4];
  char dest_floor[4];
  while (cur_int != dest_int)
  {
    if (str_to_int(shm->destination_floor) > str_to_int(highest_floor))
    {
      pthread_mutex_lock(&shm->mutex);
      strcpy(shm->destination_floor, highest_floor);
      pthread_mutex_unlock(&shm->mutex);
    }
    if (dest_int > cur_int)
    {
      pthread_mutex_lock(&shm->mutex);
      strcpy(shm->status, "Between");
      pthread_mutex_unlock(&shm->mutex);
      usleep(delay);
      cur_int++;
    }
    else if (dest_int < cur_int)
    {
      pthread_mutex_lock(&shm->mutex);
      strcpy(shm->status, "Between");
      pthread_mutex_unlock(&shm->mutex);
      usleep(delay);
      cur_int--;
    }
    else if (dest_int == cur_int)
    {
      pthread_mutex_lock(&shm->mutex);
      shm->open_button = 1;
      pthread_mutex_unlock(&shm->mutex);
      pthread_cond_signal(&shm->cond);
    }
    sprintf(cur_floor, "%d", cur_int);
    pthread_mutex_lock(&shm->mutex);
    strcpy(shm->current_floor, cur_floor);
    strcpy(shm->status, "Closed");
    pthread_mutex_unlock(&shm->mutex);
  }
  pthread_exit(NULL);
}

void* open_door(void *p)
{
  car_shared_mem *shm = p;
  strcpy(shm->status, "Opening");
  usleep(delay);
  strcpy(shm->status, "Open");
  if (shm->individual_service_mode != 1)
  {
    usleep(delay);
    strcpy(shm->status, "Closing");
    usleep(delay);
    strcpy(shm->status, "Closed");
  }
  pthread_exit(NULL);
}

void* close_door(void *p)
{
  car_shared_mem *shm = p;
  strcpy(shm->status, "Closing");
  usleep(delay);
  strcpy(shm->status, "Closed");
  pthread_exit(NULL);
}

void sigint_handler(int sig)
{
  signal(sig, SIG_IGN);

  munmap(shm, sizeof(car_shared_mem));
  shm_unlink(mem_name);
  shm = NULL;
}
