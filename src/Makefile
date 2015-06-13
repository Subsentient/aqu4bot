CC = gcc
CXX = g++
USECPP = NO

ifeq ($(USECPP),YES)
	RCC = $(CXX)
	LANGSTD = 
else
	RCC = $(CC)
	LANGSTD = -std=gnu99
endif

OUTFILE = aqu4bot
OS = UNIX
AQ_CORE_OBJECTS = auth.o commands.o config.o ddg.o irc.o logging.o main.o netcore.o curlcore.o wz.o
AQ_OBJECTS = $(AQ_CORE_OBJECTS)

ifeq ($(USECURL),NO)
	CFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DNO_LIBCURL -DBOT_OS="\"UNIX (without libcurl)\""
	AQ_CORE_OBJECTS = auth.o commands.o config.o irc.o logging.o main.o netcore.o wz.o
	AQ_OBJECTS = $(AQ_CORE_OBJECTS)
	LDFLAGS =
else
	CFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0
	LDFLAGS = -lcurl
endif

ifeq ($(OS),WIN)
	
	ifeq ($(USECURL),NO)
		LDFLAGS = -lws2_32
		CFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DWIN -DBOT_OS="\"Windows (without libcurl)\"" -DNO_LIBCURL
	else
		LDFLAGS = -lws2_32 `i686-pc-mingw32-pkg-config --libs libcurl`
		CFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DWIN -DBOT_OS=\"Windows\" -DCURL_STATICLIB
	endif

	OUTFILE = aqu4bot.exe
	RANLIB = i686-pc-mingw32-ranlib
	CC = i686-pc-mingw32-gcc
	AQ_OBJECTS = $(AQ_CORE_OBJECTS) ../win32/aqu4bot-win.o
endif

ifeq ($(OS),HAIKU)
	ifeq ($(USECURL),NO)
		CFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DHAIKU -DBOT_OS="\"Haiku OS (without libcurl)\"" -DNO_LIBCURL
		LDFLAGS = -lnetwork
	else
		LDFLAGS = -lcurl -lnetwork
		CFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DHAIKU -DBOT_OS="\"Haiku OS\""
	endif

endif

all:

ifeq ($(OS),WIN)
	$(MAKE) -C ../win32
endif
	$(RCC) auth.c -c $(CFLAGS)
	$(RCC) commands.c -c $(CFLAGS)
	$(RCC) config.c -c $(CFLAGS)
	$(RCC) irc.c -c $(CFLAGS)
	$(RCC) logging.c -c $(CFLAGS)
	$(RCC) main.c -c $(CFLAGS)
	$(RCC) netcore.c -c $(CFLAGS)
	$(RCC) wz.c -c $(CFLAGS)
ifneq ($(USECURL),NO)
	$(RCC) curlcore.c -c $(CFLAGS)
	$(RCC) ddg.c -c $(CFLAGS)
endif

ifeq ($(OS),WIN)
	$(MAKE) -C substrings static RANLIB=$(RANLIB) CC=$(CC)
else
	$(MAKE) -C substrings static

endif
	$(RCC) $(CFLAGS) -o ../$(OUTFILE) $(AQ_OBJECTS) substrings/libsubstrings.a $(LDFLAGS)

clean:
	rm -vf *.o
	$(MAKE) -C substrings clean
