CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mincc: $(OBJS)
		$(CC) -o mincc $(OBJS) $(LDFLAGS)



test: mincc
		./test.sh

clean:
		rm -f mincc *~ tmp* *.o

.PHONY: test clean