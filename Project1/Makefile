CC=gcc
CFLAGS=-I -Wall -Werror -ggdb3
EXECUTABLE = pseudo-shell
HEADERS=command.h string_parser.h
SRCS= main.c command.c string_parser.c
OBJS= $(SRCS:.c=.o)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(EXECUTABLE)

.PHONY: all clean
