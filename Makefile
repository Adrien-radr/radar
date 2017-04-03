.PHONY: tags lib radar clean post_build

all: tags radar lib post_build

##################################################
##################################################
# LIBRARY Defines
GLFW_INCLUDE=ext/glfw/include
GLFW_LIB=ext\glfw\build\x64\Release
OPENAL_INCLUDE=ext/openal-soft/include
OPENAL_LIB=ext/openal-soft/build/Release

# Default Flags
# MSVC :
CC=cl /nologo
STATICLIB=lib /nologo
LINK=/link /MACHINE:X64 -subsystem:console,5.02
GENERAL_CFLAGS=-Gm- -EHa- -GR- -EHsc
CFLAGS=-MT $(GENERAL_CFLAGS)
DLL_CFLAGS=-MD $(GENERAL_CFLAGS)
DEBUG_FLAGS=-DDEBUG -Zi -Od -WX -W1 -wd4100 -wd4189 -wd4514
RELEASE_FLAGS=-O2 -Oi

TARGET=bin/radar.exe
PDB_TARGET=bin/radar.pdb

# GCC :
#CC=g++
#STATICLIB=ar rcs
#CFLAGS=-g -Wno-unused-variable -Wno-unused-parameter -fno-exceptions -D_CRT_SECURE_NO_WARNINGS
#TARGET=bin/radar

#DEBUG_FLAGS=-DDEBUG -Wall -Wextra
#RELEASE_FLAGS=-O2

VERSION_FLAGS=$(DEBUG_FLAGS)

##################################################
##################################################
# GLEW
GLEW_OBJECT=ext/glew/glew.obj
GLEW_TARGET=ext/glew/glew.lib
GLEW_LIB=ext/glew
GLEW_INCLUDE=ext/glew/include

$(GLEW_TARGET): 
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -DGLEW_STATIC -I$(GLEW_INCLUDE) -c ext/glew/src/glew.c -Fo$(GLEW_OBJECT)
	$(STATICLIB) $(GLEW_OBJECT) -OUT:$(GLEW_TARGET)
	rm $(GLEW_OBJECT)

##################################################
# CJSON
CJSON_OBJECT=ext/cjson/cjson.obj
CJSON_TARGET=ext/cjson/cjson.lib
CJSON_LIB=ext/cjson
CJSON_INCLUDE=ext/cjson

$(CJSON_TARGET):
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -I$(CJSON_INCLUDE) -c ext/cjson/cjson.c -Fo$(CJSON_OBJECT)
	$(STATICLIB) $(CJSON_OBJECT) -OUT:$(CJSON_TARGET)
	rm $(CJSON_OBJECT)

##################################################
# Game Lib
LIB_TARGET=bin/sun.dll
LIB_SRCS=\
		 sun.cpp
LIB_INCLUDES=\
			 sun.h

lib:
	@$(CC) $(DLL_CFLAGS) $(VERSION_FLAGS) /LD $(LIB_SRCS) $(LINK) /OUT:$(LIB_TARGET)
	@#GCC : $(CC) $(CFLAGS) $(VERSION_FLAGS) -shared $(LIB_SRCS) -o $(LIB_TARGET)

##################################################
# Platform Application
SRCS=\
	 radar.cpp
INCLUDES=\
		 radar.h

radar: $(GLEW_TARGET) $(CJSON_TARGET)
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) -DGLEW_STATIC $(SRCS) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -I$(OPENAL_INCLUDE) -I$(CJSON_INCLUDE) $(LINK) /LIBPATH:$(OPENAL_LIB) /LIBPATH:$(GLEW_LIB) /LIBPATH:$(GLFW_LIB) /LIBPATH:$(CJSON_LIB) cjson.lib OpenAL32.lib libglfw3.lib glew.lib opengl32.lib user32.lib shell32.lib gdi32.lib /OUT:$(TARGET) /PDB:$(PDB_TARGET)

##################################################

post_build:
	@mv *.pdb bin/
	@rm *.obj *.lib *.exp

clean:
	rm $(TARGET)
	rm $(LIB_TARGET)

tags:
	@ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRCS) $(LIB_SRCS) $(INCLUDES) $(LIB_INCLUDES)
