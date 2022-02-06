CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mincc: $(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): mincc.h

test: mincc
		./mincc tests > tmp.s
		echo 'int char_fn() { return 257; }' | gcc -xc -c -o tmp2.o -
		gcc -static -o tmp tmp.s tmp2.o
		./tmp

clean:
		rm -f mincc *~ tmp* *.o

.PHONY: test clean
