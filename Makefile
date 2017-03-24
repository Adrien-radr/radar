CC = g++
CFLAGS = -g -Wall -Wextra -D_DEBUG -Wno-unused-variable -Wno-unused-parameter
TARGET = bin/radar

SRCS = \
	   radar.cpp

.PHONY: lib radar

all: lib radar

lib:
	$(CC) -shared sun.cpp $(CFLAGS) -o bin/sun.dll

radar:
	$(CC) $(CFLAGS) $(SRCS) -Lbin -lsun -o $(TARGET)
