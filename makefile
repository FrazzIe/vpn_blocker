CC = g++
TARGET_NAME = vpn_blocker

ifeq ($(OS), Windows_NT)
	EXT = dll
	CFLAGS = -m32 -Wall -O1 -g -mtune=core2
	LFLAGS = -m32 -g -shared -static-libgcc -static-libstdc++
	LIBS = -L.. -lcom_plugin
else
	EXT = so
	CFLAGS = -m32 -Wall -O1 -g -fvisibility=hidden -mtune=core2
	LFLAGS = -m32 -g -shared
	LIBS =
endif

all: main.cpp whitelist.cpp ipintel.cpp
	$(CC) $(CFLAGS) -c *.cpp
	$(CC) $(LFLAGS) -o $(TARGET_NAME).$(EXT) *.o $(LIBS)
clean:
	rm *.o