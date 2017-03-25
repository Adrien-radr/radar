.PHONY: tags lib radar clean

all: tags radar lib 

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
GLEW_OBJECT=ext/glew/glew.o
GLEW_TARGET=ext/glew/libglew.a
GLEW_LIB=ext/glew
GLEW_INCLUDE=ext/glew/include

$(GLEW_TARGET): 
	$(CC) -O2 -DGLEW_STATIC -I$(GLEW_INCLUDE) -c ext/glew/src/glew.c -o $(GLEW_OBJECT)
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
	$(CC) -shared $(LIB_SRCS) $(CFLAGS) -o $(LIB_TARGET)

##################################################
# Platform Application
TARGET=bin/radar
SRCS=\
	 radar.cpp
INCLUDES=\
		 radar.h

radar: $(GLEW_TARGET)
	$(CC) $(CFLAGS) -DGLEW_STATIC $(SRCS) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -L$(GLEW_LIB) -L$(GLFW_LIB) -lglfw3 -lglew -lopengl32 -lgdi32 -o $(TARGET)

##################################################

clean:
	rm $(TARGET)
	rm $(LIB_TARGET)

tags:
	ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRCS) $(LIB_SRCS) $(INCLUDES) $(LIB_INCLUDES)
