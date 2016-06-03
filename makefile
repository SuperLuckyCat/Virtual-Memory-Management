CC = gcc
CFLAGS = -I. -Wall
DEPS = mmu_defines.h
OBJ = pager_functions_v2.o pager_testing.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

pager: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)