CFLAGS=-pthread

all: car internal call

car: car.c car_mem_struct.h 
	gcc -o car car.c -pthread
internal: internal.c car_mem_struct.h 
	gcc -o internal internal.c -pthread
call: call.c
	gcc -o call call.c
