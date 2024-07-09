CFLAGS=-Wall -Werror -O2
LFLAGS=-lm

all: brutedet

debug: CFLAGS += -DDEBUG -g
debug: brutedet

brutedet: utils.o buffer.o murmur.o time.o brutedet.o
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@	

clean:
	$(RM) brutedet *.o core core.*
