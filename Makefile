CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mincc: $(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): mincc.h

test: mincc
		./test.sh

clean:
		rm -f mincc *~ tmp* *.o

.PHONY: test clean