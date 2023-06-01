CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99

all: mmu_test

mmu_test: mmu.o framelist.o mmu_test.o
	$(CC) $(CFLAGS) -o mmu_test mmu.o framelist.o mmu_test.o

mmu.o: mmu.c mmu.h
	$(CC) $(CFLAGS) -c mmu.c

framelist.o: framelist.c framelist.h mmu.h
	$(CC) $(CFLAGS) -c framelist.c

mmu_test.o: mmu_test.c mmu.h
	$(CC) $(CFLAGS) -c mmu_test.c

clean:
	rm -f mmu_test *.o

valgrind: CFLAGS += -g
valgrind: clean mmu_test