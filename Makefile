.PHONY: lib radar clean

all: lib radar

##################################################
##################################################
# LIBRARY Defines
GLFW_INCLUDE=ext/glfw/include
GLFW_LIB=ext/glfw/build/src/

# Default Flags
CC = g++
CFLAGS = -g -Wall -Wextra -D_DEBUG -Wno-unused-variable -Wno-unused-parameter

##################################################
##################################################
# GLEW
GLEW_TARGET=ext/glew/glew.o

$(GLEW_TARGET): 
	$(CC) -O2 -DGLEW_STATIC -Iext/glew/include -c ext/glew/src/glew.c -o $(GLEW_TARGET)

##################################################
# Game Lib
LIB_TARGET=bin/sun.dll
LIB_SRCS=\
		 sun.cpp
lib:
	$(CC) -shared $(LIB_SRCS) $(CFLAGS) -o $(LIB_TARGET)

##################################################
# Platform Application
TARGET=bin/radar
SRCS=\
	 radar.cpp

radar: $(GLEW_TARGET)
	$(CC) $(CFLAGS) $(SRCS) $(GLEW_TARGET) -I$(GLFW_INCLUDE) -L$(GLFW_LIB) -lglfw3 -lopengl32 -lgdi32 -o $(TARGET)

##################################################

clean:
	rm $(TARGET)
	rm $(LIB_TARGET)
