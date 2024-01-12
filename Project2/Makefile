CC = gcc
CFLAGS = -g -W -Wall
SRCS = iobound.c cpubound.c part1.c part2.c part3.c part4.c part5.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = $(SRCS:.c=)

all: $(EXECUTABLE)

$(EXECTUABLE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(OBJS) $(EXECUTABLE)
	
.PHONY: all clean

