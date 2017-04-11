.PHONY: tags lib radar clean post_build

all: tags radar lib post_build

# LIBRARY Defines
GLFW_INCLUDE=ext/glfw/include
OPENAL_INCLUDE=ext/openal-soft/include
GLEW_INCLUDE=ext/glew/include
CJSON_INCLUDE=ext/cjson
GLEW_LIB=ext/glew
CJSON_LIB=ext/cjson

SRCS=radar.cpp
INCLUDES=radar.h

LIB_SRCS=sun.cpp
LIB_INCLUDES=sun.h

##################################################
# NOTE - WINDOWS BUILD
##################################################
ifeq ($(OS),Windows_NT) # MSVC
GLEW_OBJECT=ext/glew/glew.obj
GLEW_TARGET=ext/glew/glew.lib
CJSON_OBJECT=ext/cjson/cjson.obj
CJSON_TARGET=ext/cjson/cJSON.lib
OPENAL_LIB=ext/openal-soft/build/Release
GLFW_LIB=ext/glfw/build/x64/Release

CC=cl /nologo
STATICLIB=lib /nologo
LINK=/link /MACHINE:X64 -subsystem:console,5.02 /INCREMENTAL:NO
GENERAL_CFLAGS=-Gm- -EHa- -GR- -EHsc
CFLAGS=-MT $(GENERAL_CFLAGS)
DLL_CFLAGS=-MD $(GENERAL_CFLAGS)
DEBUG_FLAGS=-DDEBUG -Zi -Od -WX -W1 -wd4100 -wd4189 -wd4514
RELEASE_FLAGS=-O2 -Oi
VERSION_FLAGS=$(DEBUG_FLAGS)

LIB_FLAGS=/LIBPATH:$(OPENAL_LIB) /LIBPATH:$(GLEW_LIB) /LIBPATH:$(GLFW_LIB) /LIBPATH:$(CJSON_LIB) cjson.lib OpenAL32.lib libglfw3.lib glew.lib opengl32.lib user32.lib shell32.lib gdi32.lib

TARGET=bin/radar.exe
LIB_TARGET=bin/sun.dll
PDB_TARGET=bin/radar.pdb


$(GLEW_TARGET): 
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -DGLEW_STATIC -I$(GLEW_INCLUDE) -c ext/glew/src/glew.c -Fo$(GLEW_OBJECT)
	@$(STATICLIB) $(GLEW_OBJECT) -OUT:$(GLEW_TARGET)
	@rm $(GLEW_OBJECT)


$(CJSON_TARGET):
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -I$(CJSON_INCLUDE) -c ext/cjson/cjson.c -Fo$(CJSON_OBJECT)
	@$(STATICLIB) $(CJSON_OBJECT) -OUT:$(CJSON_TARGET)
	@rm $(CJSON_OBJECT)


lib:
	@$(CC) $(DLL_CFLAGS) $(VERSION_FLAGS) /LD $(LIB_SRCS) $(LINK) /OUT:$(LIB_TARGET)
	@mv *.pdb bin/

radar: $(GLEW_TARGET) $(CJSON_TARGET)
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) -DGLEW_STATIC $(SRCS) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -I$(OPENAL_INCLUDE) -I$(CJSON_INCLUDE) $(LINK) $(LIB_FLAGS) /OUT:$(TARGET) /PDB:$(PDB_TARGET)

##################################################
# NOTE - LINUX BUILD
##################################################
else # GCC
GLEW_OBJECT=ext/glew/glew.o
GLEW_TARGET=ext/glew/libglew.lib
CJSON_OBJECT=ext/cjson/cJSON.o
CJSON_TARGET=ext/cjson/libcJSON.lib
OPENAL_LIB=ext/openal-soft/build
GLFW_LIB=ext/glfw/build/src

CC=g++
CFLAGS=-g -Wno-unused-variable -Wno-unused-parameter -Wno-write-strings -fno-exceptions -D_CRT_SECURE_NO_WARNINGS
DEBUG_FLAGS=-DDEBUG -Wall -Wextra
RELEASE_FLAGS=-O2
VERSION_FLAGS=$(RELEASE_FLAGS)

LIB_FLAGS=-L$(OPENAL_LIB) -L$(GLEW_LIB) -L$(GLFW_LIB) -L$(CJSON_LIB) -lcJSON -lglfw3 -lglew -lopenal \
		  -lGL -lX11 -lXinerama -lXrandr -lXcursor -ldl -lpthread

TARGET=bin/radar
LIB_TARGET=bin/sun.so

$(GLEW_TARGET): 
	@echo "AR $(GLEW_TARGET)"
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -DGLEW_STATIC -I$(GLEW_INCLUDE) -c ext/glew/src/glew.c -o $(GLEW_OBJECT)
	@ar rcs $(GLEW_TARGET) $(GLEW_OBJECT)
	@rm $(GLEW_OBJECT)

$(CJSON_TARGET):
	@echo "AR $(GLEW_TARGET)"
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -I$(CJSON_INCLUDE) -c ext/cjson/cJSON.c -o $(CJSON_OBJECT)
	@ar rcs $(CJSON_TARGET) $(CJSON_OBJECT) 
	@rm $(CJSON_OBJECT)

lib:
	@echo "LIB $(LIB_TARGET)"
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) -shared -fPIC $(LIB_SRCS) -o $(LIB_TARGET)

radar: $(GLEW_TARGET) $(CJSON_TARGET)
	@echo "CC $(TARGET)"
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) -DGLEW_STATIC $(SRCS) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -I$(OPENAL_INCLUDE) -I$(CJSON_INCLUDE) $(LIB_FLAGS) -o $(TARGET)

endif
#$(error OS not compatible. Only Win32 and Linux for now.)

##################################################
##################################################

post_build:
	@rm -f *.obj *.lib *.exp

clean:
	rm $(TARGET)
	rm $(LIB_TARGET)

tags:
	@ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRCS) $(LIB_SRCS) $(INCLUDES) $(LIB_INCLUDES)
