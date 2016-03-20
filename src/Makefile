CC = gcc
CXX = g++
LANGSTD = -std=gnu++98


OUTFILE = aqu4bot
OS = UNIX
AQ_CORE_OBJECTS = auth.o commands.o config.o ddg.o irc.o logging.o main.o netcore.o curlcore.o wz.o
AQ_OBJECTS = $(AQ_CORE_OBJECTS)

ifeq ($(USECURL),NO)
	CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DNO_LIBCURL -DBOT_OS="\"UNIX (without libcurl)\""
	AQ_CORE_OBJECTS = auth.o commands.o config.o irc.o logging.o main.o netcore.o wz.o
	AQ_OBJECTS = $(AQ_CORE_OBJECTS)
	LDFLAGS =
else
	CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0
	LDFLAGS = -lcurl
endif

ifeq ($(OS),WIN)
	
	ifeq ($(USECURL),NO)
		LDFLAGS = -lws2_32
		CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DWIN -DBOT_OS="\"Windows (without libcurl)\"" -DNO_LIBCURL
	else
		LDFLAGS = -lws2_32 `i686-pc-mingw32-pkg-config --libs libcurl`
		CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DWIN -DBOT_OS=\"Windows\" -DCURL_STATICLIB
	endif

	OUTFILE = aqu4bot.exe
	RANLIB = i686-pc-mingw32-ranlib
	CC = i686-pc-mingw32-gcc
	CXX = i686-pc-mingw32-g++
	AQ_OBJECTS = $(AQ_CORE_OBJECTS) ../win32/aqu4bot-win.o
endif

ifeq ($(OS),HAIKU)
	ifeq ($(USECURL),NO)
		CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DHAIKU -DBOT_OS="\"Haiku OS (without libcurl)\"" -DNO_LIBCURL
		LDFLAGS = -lnetwork
	else
		LDFLAGS = -lcurl -lnetwork
		CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DHAIKU -DBOT_OS="\"Haiku OS\""
	endif

endif

all:

ifeq ($(OS),WIN)
	$(MAKE) -C ../win32
endif
	$(CXX) auth.c -c $(CXXFLAGS)
	$(CXX) commands.c -c $(CXXFLAGS)
	$(CXX) config.c -c $(CXXFLAGS)
	$(CXX) irc.c -c $(CXXFLAGS)
	$(CXX) logging.c -c $(CXXFLAGS)
	$(CXX) main.c -c $(CXXFLAGS)
	$(CXX) netcore.c -c $(CXXFLAGS)
	$(CXX) wz.c -c $(CXXFLAGS)
ifneq ($(USECURL),NO)
	$(CXX) curlcore.c -c $(CXXFLAGS)
	$(CXX) ddg.c -c $(CXXFLAGS)
endif

ifeq ($(OS),WIN)
	$(MAKE) -C substrings static RANLIB=$(RANLIB) CC=$(CC)
else
	$(MAKE) -C substrings static

endif
	$(CXX) $(CXXFLAGS) -o ../$(OUTFILE) $(AQ_OBJECTS) substrings/libsubstrings.a $(LDFLAGS)

clean:
	rm -vf *.o
	$(MAKE) -C substrings clean
