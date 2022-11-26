CC=gcc
CFLAGS=-std=c11 -lncurses
TARGETS=src/*.c
OUT=bin/text2_0

all:
	$(CC) $(CFLAGS) $(TARGETS) -o $(OUT)

clean:
	rm -f $(OUT)

test:
	$(MAKE) all
	./$(OUT)
	$(MAKE) clean
