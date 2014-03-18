CFLAGS = -std=gnu89 -pedantic -Wall -g3 -O0
CC = gcc

all:
	$(CC) auth.c -c $(CFLAGS)
	$(CC) commands.c -c $(CFLAGS)
	$(CC) config.c -c $(CFLAGS)
	$(CC) irc.c -c $(CFLAGS)
	$(CC) logging.c -c $(CFLAGS)
	$(CC) main.c -c $(CFLAGS)
	$(CC) netcore.c -c $(CFLAGS)
	$(CC) wz.c -c $(CFLAGS)
	
	$(MAKE) -C substrings static

	$(CC) $(CFLAGS) -o ../aqu4bot auth.o commands.o config.o irc.o logging.o main.o netcore.o wz.o substrings/libsubstrings.a

clean:
	rm -vf *.o
	$(MAKE) -C substrings clean
