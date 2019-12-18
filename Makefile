CC = gcc
CFLAGS = -g -Wall -ansi -pedantic
OBJS = shell.o jobs_executor.o\
	interface_controller.o\
	shell_controller.o\
	stream_handler.o\
	miscellaneous_stuff.o
	
BINARIES = sos

all: $(BINARIES)

sos: $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(OBJS) $(BINARIES)

