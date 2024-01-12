CC = gcc
CFLAGS = -g -W -Wall
SRCS = bank.c
HEADERS = account.h
OBJS = $(SRCS:.c=.o)
EXECUTABLE = $(SRCS:.c=)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(OBJS) $(EXECUTABLE)

.PHONY: clean
