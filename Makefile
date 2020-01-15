CC = gcc
CFLAGS = -W -g -fsanitize=address

all: ftl

ftl: main.c ftl_box.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm ftl
