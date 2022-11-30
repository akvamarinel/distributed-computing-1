CC = gcc
CFLAGS += -std=c99 -Wall -pedantic -g -I.
SRCS = $(shell find . -type f -name "*.c")

all: pa23

pa23: $(SRCS)
	$(CC) $(CFLAGS) -L. $^ -o $@ -lruntime

clean:
	rm -rf pa23
