CC = gcc
CFLAGS = -g -Wall
MYMALLOC_SRC = mysh.c
SRCS = mysh.c
TARGETS = mysh

all: $(TARGETS)

mysh: mysh.o 
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS) $(wildcard *.o)
