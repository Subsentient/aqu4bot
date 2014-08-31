CFLAGS = -std=gnu89 -pedantic -Wall -g3 -O0
LDFLAGS = -lcurl
CC = gcc
OUTFILE = aqu4bot
OS = UNIX

ifeq ($(OS),WIN)
	CFLAGS = -std=gnu89 -pedantic -Wall -g3 -O0 -DWIN -DBOT_OS=\"Windows\" -DCURL_STATICLIB
	LDFLAGS = -lws2_32 `i686-pc-mingw32-pkg-config --libs libcurl`
	OUTFILE = aqu4bot.exe
	RANLIB = i686-pc-mingw32-ranlib
	CC = i686-pc-mingw32-gcc
endif

all:
	$(CC) auth.c -c $(CFLAGS)
	$(CC) commands.c -c $(CFLAGS)
	$(CC) config.c -c $(CFLAGS)
	$(CC) ddg.c -c $(CFLAGS)
	$(CC) irc.c -c $(CFLAGS)
	$(CC) logging.c -c $(CFLAGS)
	$(CC) main.c -c $(CFLAGS)
	$(CC) netcore.c -c $(CFLAGS)
	$(CC) curlcore.c -c $(CFLAGS)
	$(CC) wz.c -c $(CFLAGS)
ifeq ($(OS),WIN)
	$(MAKE) -C substrings static RANLIB=$(RANLIB) CC=$(CC)
else
	$(MAKE) -C substrings static
endif
	$(CC) $(CFLAGS) -o ../$(OUTFILE) auth.o commands.o config.o ddg.o irc.o logging.o main.o netcore.o curlcore.o wz.o substrings/libsubstrings.a $(LDFLAGS)

clean:
	rm -vf *.o
	$(MAKE) -C substrings clean
