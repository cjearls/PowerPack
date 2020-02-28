SHELL = /bin/sh

CXX = g++
CXXFLAGS += -std=c++11 -fPIC

###########NIDAQMAX RELATED FLAGS/VARIABLES
LIBS = nidaqmx

OS := $(shell uname)

ifeq ($(OS),Darwin)
LIBFLAGS = -framework $(LIBS)
else
LIBFLAGS = -l$(LIBS)
LDFLAGS += -L/usr/lib/x86_64-linux-gnu
endif

LIBFLAGS += -lm
LDFLAGS += -g

ifneq ($(filter $(OS), Linux Darwin),)
CXXFLAGS += -D_POSIX_C_SOURCE=200809L
endif

ifeq ($(OS),Darwin)
# Using #include <NIDAQmx.h> requires NIDAQmx.h to be in the include path.
# Using #include <nidaqmx/NIDAQmx.h> takes advantage of the fact that nidaqmx.framework/Headers contains NIDAQmx.h, but this only works on OS X.
NIDAQmx_HEADER_DIR = /Library/Frameworks/nidaqmx.framework/Headers
CXXFLAGS += -I$(NIDAQmx_HEADER_DIR) -arch i386 -arch x86_64
LDFLAGS += -arch i386 -arch x86_64
endif
###########

OBJS = timeutils.o eventhandler.o socketutils.o functionapi.o 
NIDAQOBJS = nidaqmxeventhandler.o

all: example

debug:	CXXFLAGS += -ggdb3 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast
debug: all

so: $(OBJS)
	$(CXX) -shared -o libpowerpack.so $(OBJS)

example: serverexample clientexample

serverexample: serverexample.o $(OBJS) $(NIDAQOBJS)
	$(CXX)  $(LDFLAGS) $(LIBFLAGS) -Wall -pthread serverexample.o $(OBJS) $(NIDAQOBJS) -o serverexample

clientexample: clientexample.o $(OBJS)
	$(CXX) $(LDFLAGS) $(LIBFLAGS) -Wall -pthread clientexample.o $(OBJS) -o clientexample

clientexample.o: functionapi.h
serverexample.o: functionapi.h nidaqmxeventhandler.h
testsockets.o: socketutils.h

functionapi.o: functionapi.h
socketutils.o: eventhandler.h socketutils.h timeutils.h
nidaqmxeventhandler.o: eventhandler.h nidaqmxeventhandler.h
eventhandler.o: eventhandler.h
timeutils.o: timeutils.h


.PHONY: clean
clean:
	rm -f *.o valgrind.log %1 core clientexample serverexample testsockets
