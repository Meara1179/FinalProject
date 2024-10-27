CFLAGS=-pthread

all: car controller call internal safety

car: car.c car_mem_struct.h 
	gcc -o car car.c -pthread
controller: controller.c
	gcc -o controller controller.c
call: call.c
	gcc -o call call.c
internal: internal.c car_mem_struct.h 
	gcc -o internal internal.c -pthread
safety: safety.c car_mem_struct.h
	gcc -o safety safety.c -Wall -Wextra -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings

