.PHONY: tags lib radar clean post_build

all: tags radar lib post_build

# LIBRARY Defines
GLFW_INCLUDE=ext/glfw/include
OPENAL_INCLUDE=ext/openal-soft/include
GLEW_INCLUDE=ext/glew/include
SFMT_INCLUDE=ext/sfmt/
CJSON_INCLUDE=ext/cjson
STB_INCLUDE=ext/
GLEW_LIB=ext/glew
CJSON_LIB=ext/cjson
SFMT_LIB=ext/sfmt/

INCLUDE_FLAGS=-I$(SRD_DIR) -Iext/ -I$(SFMT_INCLUDE) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -I$(OPENAL_INCLUDE) -I$(STB_INCLUDE) -I$(CJSON_INCLUDE)

SRC_DIR=src/
SRCS= \
	sampling.cpp \
	render.cpp \
	utils.cpp \
	log.cpp \
	context.cpp \
	sound.cpp \
	ui.cpp \
	water.cpp \
	atmosphere.cpp \
	radar.cpp
OBJ_DIR=obj/
INCLUDES=radar.h

LIB_SRCS=$(SRC_DIR)sun.cpp
LIB_INCLUDES=$(SRC_DIR)sun.h

##################################################
# NOTE - WINDOWS BUILD
##################################################
ifeq ($(OS),Windows_NT) # MSVC
GLEW_OBJECT=ext/glew/glew.obj
GLEW_TARGET=ext/glew/glew.lib
CJSON_OBJECT=ext/cjson/cjson.obj
CJSON_TARGET=ext/cjson/cJSON.lib
SFMT_OBJECT=ext/sfmt/SFMT.obj
SFMT_TARGET=ext/sfmt/SFMT.lib
STB_TARGET=ext/stb.lib
STB_OBJECT=ext/stb.obj
OPENAL_LIB=ext/openal-soft/build/Release
GLFW_LIB=ext/glfw/build/x64/Release

OBJS=$(patsubst %.cpp,$(OBJ_DIR)%.obj,$(SRCS))

CC=cl /nologo
STATICLIB=lib /nologo
LINK=/link /MACHINE:X64 -subsystem:console,5.02 /INCREMENTAL:NO /ignore:4204
GENERAL_CFLAGS=-Gm- -EHa- -GR- -EHsc
CFLAGS=-MT $(GENERAL_CFLAGS) /W1 -I$(SRC_DIR)
DLL_CFLAGS=-MD -DLIBEXPORT $(GENERAL_CFLAGS)
DEBUG_FLAGS=-DDEBUG -Zi -Od -W1 -wd4100 -wd4189 -wd4514
RELEASE_FLAGS=-O2 -Oi
VERSION_FLAGS=$(DEBUG_FLAGS)

LIB_FLAGS=/LIBPATH:ext /LIBPATH:$(OPENAL_LIB) /LIBPATH:$(GLEW_LIB) /LIBPATH:$(GLFW_LIB) /LIBPATH:$(CJSON_LIB) stb.lib cjson.lib OpenAL32.lib libglfw3.lib glew.lib opengl32.lib user32.lib shell32.lib gdi32.lib

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

$(SFMT_TARGET):
	@$(CC) -O2 -Oi -DHAVE_SSE2=1 -DSFMT_MEXP=19937 -c ext/sfmt/SFMT.c -Fo$(SFMT_OBJECT)
	@$(STATICLIB) $(SFMT_OBJECT) -OUT:$(SFMT_TARGET)
	@rm $(SFMT_OBJECT)

$(STB_TARGET):
	@$(CC) -O2 -Oi -c ext/stb.cpp -Fo$(STB_OBJECT)
	@$(STATICLIB) $(STB_OBJECT) -OUT:$(STB_TARGET)
	@rm $(STB_OBJECT)

lib:
	@$(CC) $(DLL_CFLAGS) $(VERSION_FLAGS) -I$(SFMT_INCLUDE) /LD $(LIB_SRCS) $(LINK) /OUT:$(LIB_TARGET)
	@mv *.pdb bin/

$(OBJ_DIR)%.obj: $(SRC_DIR)%.cpp
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -Fo$@

radar: $(STB_TARGET) $(SFMT_TARGET) $(GLEW_TARGET) $(CJSON_TARGET) $(OBJS)
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC $(OBJS) $(LINK) $(LIB_FLAGS) /OUT:$(TARGET) /PDB:$(PDB_TARGET)

##################################################
# NOTE - LINUX BUILD
##################################################
else # GCC
GLEW_OBJECT=ext/glew/glew.o
GLEW_TARGET=ext/glew/libglew.a
CJSON_OBJECT=ext/cjson/cJSON.o
CJSON_TARGET=ext/cjson/libcJSON.a
OPENAL_LIB=ext/openal-soft/build
GLFW_LIB=ext/glfw/build/src
SFMT_OBJECT=ext/sfmt/SFMT.o
SFMT_TARGET=ext/sfmt/libSFMT.a
STB_TARGET=ext/libstb.a
STB_OBJECT=ext/stb.o

OBJS=$(patsubst %.cpp,$(OBJ_DIR)%.o,$(SRCS))

CC=g++
CFLAGS=-Wno-unused-variable -Wno-unused-parameter -Wno-write-strings -D_CRT_SECURE_NO_WARNINGS -DSFMT_MEXP=19937 -std=c++11 -pedantic
DEBUG_FLAGS=-g -DDEBUG -Wall -Wextra
RELEASE_FLAGS=-O2
VERSION_FLAGS=$(DEBUG_FLAGS)

LIB_FLAGS=-Lext/ -L$(OPENAL_LIB) -L$(GLEW_LIB) -L$(GLFW_LIB) -L$(CJSON_LIB) -L$(SFMT_LIB) -lSFMT -lstb -lcJSON -lglfw3 -lglew -lopenal \
		  -lGL -lX11 -lXinerama -lXrandr -lXcursor -lm -ldl -lpthread

TARGET=bin/radar
LIB_TARGET=bin/sun.so

$(GLEW_TARGET): 
	@echo "AR $(GLEW_TARGET)"
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -DGLEW_STATIC -I$(GLEW_INCLUDE) -c ext/glew/src/glew.c -o $(GLEW_OBJECT)
	@ar rcs $(GLEW_TARGET) $(GLEW_OBJECT)
	@rm $(GLEW_OBJECT)

$(CJSON_TARGET):
	@echo "AR $(CJSON_TARGET)"
	@$(CC) $(CFLAGS) $(RELEASE_FLAGS) -I$(CJSON_INCLUDE) -c ext/cjson/cJSON.c -o $(CJSON_OBJECT)
	@ar rcs $(CJSON_TARGET) $(CJSON_OBJECT) 
	@rm $(CJSON_OBJECT)

# Compile with specific flags for this one
$(SFMT_TARGET):
	@echo "AR $(SFMT_TARGET)"
	@$(CC) -O3 -msse2 -fno-strict-aliasing -DHAVE_SSE2=1 -DSFMT_MEXP=19937 -c ext/sfmt/SFMT.c -o $(SFMT_OBJECT)
	@ar rcs $(SFMT_TARGET) $(SFMT_OBJECT)
	@rm $(SFMT_OBJECT)

$(STB_TARGET):
	@echo "AR $(STB_TARGET)"
	@$(CC) -O3 -fno-strict-aliasing -c ext/stb.cpp -o $(STB_OBJECT)
	@ar rcs $(STB_TARGET) $(STB_OBJECT)
	@rm $(STB_OBJECT)

lib:
	@echo "LIB $(LIB_TARGET)"
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) -shared -fPIC $(LIB_SRCS) -I$(SFMT_INCLUDE) -o $(LIB_TARGET)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@echo "CC $@"
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -o $@

radar: $(STB_TARGET) $(SFMT_TARGET) $(GLEW_TARGET) $(CJSON_TARGET) $(OBJS)
	@echo "CC $(TARGET)"
	$(CC) $(CFLAGS) $(VERSION_FLAGS) -DGLEW_STATIC $(OBJS) $(INCLUDE_FLAGS) $(LIB_FLAGS) -o $(TARGET)

endif
#$(error OS not compatible. Only Win32 and Linux for now.)

##################################################
##################################################

post_build:
	@rm -f *.lib *.exp

clean:
	rm $(OBJS)
	rm $(TARGET)
	rm $(LIB_TARGET)

clean_ext:
	rm $(CJSON_TARGET)
	rm $(GLEW_TARGET)
	rm $(SFMT_TARGET)

tags:
	@ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRC_DIR)*.cpp $(SRC_DIR)*.h $(LIB_INCLUDES)
