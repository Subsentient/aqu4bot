CC = gcc
CXX = g++
LANGSTD = -std=gnu++98
STRIP = strip

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
		CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DWIN -DBOT_OS="\"Windows (without libcurl)\"" -DNO_LIBCURL -static
	else
		LDFLAGS = -lws2_32 `i686-w64-mingw32.static-pkg-config --libs libcurl` -static
		CXXFLAGS =  $(LANGSTD) -pedantic -Wall -g3 -O0 -DWIN -DBOT_OS=\"Windows\" -DCURL_STATICLIB
	endif

	OUTFILE = aqu4bot.exe
	RANLIB = i686-w64-mingw32.static-ranlib
	CC = i686-w64-mingw32.static-gcc
	CXX = i686-w64-mingw32.static-g++
	AQ_OBJECTS = $(AQ_CORE_OBJECTS) ../win32/aqu4bot-win.o
	STRIP = i686-w64-mingw32.static-strip
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
	$(MAKE) -C ../win32 WINDRES=i686-w64-mingw32.static-windres
endif
	$(CXX) auth.cpp -c $(CXXFLAGS)
	$(CXX) commands.cpp -c $(CXXFLAGS)
	$(CXX) config.cpp -c $(CXXFLAGS)
	$(CXX) irc.cpp -c $(CXXFLAGS)
	$(CXX) logging.cpp -c $(CXXFLAGS)
	$(CXX) main.cpp -c $(CXXFLAGS)
	$(CXX) netcore.cpp -c $(CXXFLAGS)
	$(CXX) wz.cpp -c $(CXXFLAGS)
ifneq ($(USECURL),NO)
	$(CXX) curlcore.cpp -c $(CXXFLAGS)
	$(CXX) ddg.cpp -c $(CXXFLAGS)
endif

ifeq ($(OS),WIN)
	$(MAKE) -C substrings static RANLIB=$(RANLIB) CC=$(CC)
else
	$(MAKE) -C substrings static

endif
	$(CXX) $(CXXFLAGS) -o ../$(OUTFILE) $(AQ_OBJECTS) substrings/libsubstrings.a $(LDFLAGS)
	$(STRIP) --strip-unneeded ../$(OUTFILE)
clean:
	rm -vf *.o
	$(MAKE) -C substrings clean
