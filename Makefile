CC = gcc

PROGS = test

all: $(PROGS)
test: test.c

clean:
	rm -rf *~ $(PROGS)
