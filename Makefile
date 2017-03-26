.PHONY: tags lib radar clean

all: tags radar lib 

##################################################
##################################################
# LIBRARY Defines
GLFW_INCLUDE=ext/glfw/include
GLFW_LIB=ext/glfw/build/src/
OPENAL_INCLUDE=ext/openal-soft/include
OPENAL_LIB=ext/openal-soft/build

# Default Flags
CC=g++
CFLAGS=-g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -D_CRT_SECURE_NO_WARNINGS

DEBUG_FLAGS=-DDEBUG
RELEASE_FLAGS=-O2
VERSION_FLAGS=$(DEBUG_FLAGS)

##################################################
##################################################
# GLEW
GLEW_OBJECT=ext/glew/glew.o
GLEW_TARGET=ext/glew/libglew.a
GLEW_LIB=ext/glew
GLEW_INCLUDE=ext/glew/include

$(GLEW_TARGET): 
	$(CC) $(RELASE_FLAGS) -DGLEW_STATIC -I$(GLEW_INCLUDE) -c ext/glew/src/glew.c -o $(GLEW_OBJECT)
	$(AR) rcs $(GLEW_TARGET) $(GLEW_OBJECT)
	rm $(GLEW_OBJECT)

##################################################
# Game Lib
LIB_TARGET=bin/sun.dll
LIB_SRCS=\
		 sun.cpp
LIB_INCLUDES=\
			 sun.h

lib:
	$(CC) $(CFLAGS) $(VERSION_FLAGS) -shared $(LIB_SRCS) -o $(LIB_TARGET)

##################################################
# Platform Application
TARGET=bin/radar
SRCS=\
	 radar.cpp
INCLUDES=\
		 radar.h

radar: $(GLEW_TARGET)
	$(CC) $(CFLAGS) $(VERSION_FLAGS) -DGLEW_STATIC $(SRCS) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -I$(OPENAL_INCLUDE) -L$(OPENAL_LIB) -L$(GLEW_LIB) -L$(GLFW_LIB) -lglfw3 -lglew -lOpenAL32 -lopengl32 -lgdi32 -o $(TARGET)

##################################################

clean:
	rm $(TARGET)
	rm $(LIB_TARGET)

tags:
	ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRCS) $(LIB_SRCS) $(INCLUDES) $(LIB_INCLUDES)
