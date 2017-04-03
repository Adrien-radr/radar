.PHONY: tags lib radar clean post_build

all: tags radar lib post_build

##################################################
##################################################
# LIBRARY Defines
GLEW_INCLUDE=ext/glew/include
GLFW_INCLUDE=ext/glfw/include
OPENAL_INCLUDE=ext/openal-soft/include
CJSON_INCLUDE=ext/cjson

GLEW_LIB=ext/glew
CJSON_LIB=ext/cjson

# Default Flags
# MSVC :
MSVC_CC=cl /nologo
MSVC_STATICLIB=lib /nologo
MSVC_LINK=/link /MACHINE:X64 -subsystem:console,5.02 /PDB:bin/radar.pdb
MSVC_GENERAL_CFLAGS=-Gm- -EHa- -GR- -EHsc
MSVC_CFLAGS=-MT $(MSVC_GENERAL_CFLAGS)
MSVC_DLL_CFLAGS=-MD $(MSVC_GENERAL_CFLAGS) /LD
MSVC_DEBUG_FLAGS=-DDEBUG -Zi -Od -WX -W1 -wd4100 -wd4189 -wd4514
MSVC_RELEASE_FLAGS=-O2 -Oi

MSVC_LIBOUT=-Fo
MSVC_OUT=/OUT:

MSVC_LIBPRE=
MSVC_OBJEXT=obj
MSVC_LIBEXT=lib
MSVC_DLLEXT=dll

MSVC_TARGET=bin/radar.exe

MSVC_OPENAL_LIB=ext/openal-soft/build/Release
MSVC_GLFW_LIB=ext/glfw/build/x64/Release

MSVC_LIB=/LIBPATH:$(MSVC_OPENAL_LIB) /LIBPATH:$(GLEW_LIB) /LIBPATH:$(GLFW_LIB) /LIBPATH:$(CJSON_LIB) cjson.lib OpenAL32.lib libglfw3.lib glew.lib opengl32.lib user32.lib shell32.lib gdi32.lib

# GCC :
GCC_CC=g++
GCC_STATICLIB=ar rcs
GCC_LINK=
GCC_GENERAL_CFLAGS=-g -Wno-unused-variable -Wno-unused-parameter -fno-exceptions -D_CRT_SECURE_NO_WARNINGS
GCC_CFLAGS=$(GCC_GENERAL_CFLAGS)
GCC_DLL_CFLAGS=$(GCC_GENERAL_CFLAGS) -shared -fPIC
GCC_DEBUG_FLAGS=-DDEBUG -Wall -Wextra
GCC_RELEASE_FLAGS=-O2

GCC_LIBOUT=-o 
GCC_OUT=-o 

GCC_LIBPRE=lib
GCC_OBJEXT=o
GCC_LIBEXT=a
GCC_DLLEXT=so

GCC_TARGET=bin/radar

GCC_OPENAL_LIB=ext/openal-soft/build
GCC_GLFW_LIB=ext/glfw/build/src

GCC_LIB=-L$(GCC_OPENAL_LIB) -L$(GLEW_LIB) -L$(GCC_GLFW_LIB) -L$(CJSON_LIB) -lcJSON -lglfw3 -lglew -lopenal \
		-lGL -lX11 -lXinerama -lXrandr -lXcursor -ldl -lpthread

ifeq ($(OS),Windows_NT)

UNAME_S="ww"
CC=$(MSVC_CC)
STATICLIB=$(MSVC_STATICLIB)
LINK=$(MSVC_LINK)
GENERAL_CFLAGS=$(MSVC_GENERAL_CFLAGS)
CFLAGS=$(MSVC_CFLAGS)
DLL_CFLAGS=$(MSVC_DLL_CFLAGS)
DEBUG_FLAGS=$(MSVC_DEBUG_FLAGS)
RELEASE_FLAGS=$(MSVC_RELEASE_FLAGS)
LIBOUT=$(MSVC_LIBOUT)
OUT=$(MSVC_OUT)
STATICLIBOUT=$(OUT)
TARGET=$(MSVC_TARGET)
LIBPRE=$(MSVC_LIBPRE)
OBJEXT=$(MSVC_OBJEXT)
LIBEXT=$(MSVC_LIBEXT)
DLLEXT=$(MSVC_DLLEXT)
LIB=$(MSVC_LIB)

else

UNAME_S=$(shell uname -s)
ifeq ($(UNAME_S),Linux)
CC=$(GCC_CC)
STATICLIB=$(GCC_STATICLIB)
LINK=$(GCC_LINK)
GENERAL_CFLAGS=$(GCC_GENERAL_CFLAGS)
CFLAGS=$(GCC_CFLAGS)
DLL_CFLAGS=$(GCC_DLL_CFLAGS)
DEBUG_FLAGS=$(GCC_DEBUG_FLAGS)
RELEASE_FLAGS=$(GCC_RELEASE_FLAGS)
LIBOUT=$(GCC_LIBOUT)
OUT=$(GCC_OUT)
STATICLIBOUT=
TARGET=$(GCC_TARGET)
LIBPRE=$(GCC_LIBPRE)
OBJEXT=$(GCC_OBJEXT)
LIBEXT=$(GCC_LIBEXT)
DLLEXT=$(GCC_DLLEXT)
LIB=$(GCC_LIB)

else
$(error OS not compatible. Only Win32 and Linux for now.)
endif

endif

VERSION_FLAGS=$(DEBUG_FLAGS)

test:
	@echo $(MSVC_OUT)$(UNAME_S)

##################################################
##################################################
# GLEW
GLEW_OBJECT=ext/glew/glew.$(OBJEXT)
GLEW_TARGET=ext/glew/$(LIBPRE)glew.$(LIBEXT)

$(GLEW_TARGET): 
	@echo "LIB $(GLEW_TARGET)"
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -DGLEW_STATIC -I$(GLEW_INCLUDE) -c ext/glew/src/glew.c $(LIBOUT)$(GLEW_OBJECT)
	@$(STATICLIB) $(STATICLIBOUT)$(GLEW_TARGET) $(GLEW_OBJECT)
	@rm $(GLEW_OBJECT)

##################################################
# CJSON
CJSON_OBJECT=ext/cjson/cJSON.$(OBJEXT)
CJSON_TARGET=ext/cjson/$(LIBPRE)cJSON.$(LIBEXT)

$(CJSON_TARGET):
	@echo "LIB $(CJSON_TARGET)"
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -I$(CJSON_INCLUDE) -c ext/cjson/cJSON.c $(LIBOUT)$(CJSON_OBJECT)
	@$(STATICLIB) $(STATICLIBOUT)$(CJSON_TARGET) $(CJSON_OBJECT)
	@rm $(CJSON_OBJECT)

##################################################
# Game Lib
LIB_TARGET=bin/sun.$(DLLEXT)
LIB_SRCS=\
		 sun.cpp
LIB_INCLUDES=\
			 sun.h

lib:
	@echo "DLL $(LIB_TARGET)"
	@$(CC) $(DLL_CFLAGS) $(VERSION_FLAGS) $(LIB_SRCS) $(LINK) $(OUT)$(LIB_TARGET)

##################################################
# Platform Application
SRCS=\
	 radar.cpp
INCLUDES=\
		 radar.h

radar: $(GLEW_TARGET) $(CJSON_TARGET)
	$(CC) $(CFLAGS) $(VERSION_FLAGS) -DGLEW_STATIC $(SRCS) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -I$(OPENAL_INCLUDE) -I$(CJSON_INCLUDE) $(LINK) $(LIB) $(OUT)$(TARGET)

##################################################

post_build:
	@mv *.pdb bin/
	@rm *.obj *.lib *.exp

clean:
	rm $(LIB_TARGET)
	rm $(TARGET)

tags:
	@ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRCS) $(LIB_SRCS) $(INCLUDES) $(LIB_INCLUDES)
