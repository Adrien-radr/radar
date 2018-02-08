.PHONY: tags lib radar clean post_build

all: tags pre_build radar post_build

RF_DIR=ext/rf/

# LIBRARY Defines
GLFW_INCLUDE=$(RF_DIR)ext/glfw/include
GLEW_INCLUDE=$(RF_DIR)ext/glew/include
CJSON_INCLUDE=$(RF_DIR)ext/cjson
STB_INCLUDE=$(RF_DIR)ext
SFMT_INCLUDE=ext/sfmt
OPENAL_INCLUDE=ext/openal-soft/include
GLEW_LIB=$(RF_DIR)ext/glew
CJSON_LIB=$(RF_DIR)ext/cjson
SFMT_LIB=ext/sfmt
RF_INCLUDE=$(RF_DIR)include
RF_LIB=$(RF_DIR)lib

SRC_DIR=src/

INCLUDE_FLAGS=-I$(SRC_DIR) -Iext/ -I$(SFMT_INCLUDE) -I$(OPENAL_INCLUDE) -I$(CJSON_INCLUDE) -I$(GLEW_INCLUDE) -I$(GLFW_INCLUDE) -I$(RF_INCLUDE)

SRCS= \
	sound.cpp \
	sampling.cpp \
	tests.cpp \
	Game/sun.cpp \
	Systems/atmosphere.cpp \
	Systems/water.cpp \
	radar.cpp 
OBJ_DIR=obj/
INCLUDES=radar.h

##################################################
# NOTE - WINDOWS BUILD
##################################################
ifeq ($(OS),Windows_NT) # MSVC
GLFW_LIB=$(RF_DIR)ext/glfw/build/x64/Release
SFMT_OBJECT=ext/sfmt/SFMT.obj
SFMT_TARGET=ext/sfmt/SFMT.lib
OPENAL_LIB=ext/openal-soft/build/Release

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

LIB_FLAGS=/LIBPATH:ext /LIBPATH:$(RF_DIR)/ext /LIBPATH:$(OPENAL_LIB) /LIBPATH:$(GLEW_LIB) /LIBPATH:$(GLFW_LIB) /LIBPATH:$(CJSON_LIB) /LIBPATH:$(RF_LIB) rf.lib stb.lib cjson.lib OpenAL32.lib libglfw3.lib glew.lib opengl32.lib user32.lib shell32.lib gdi32.lib

TARGET=bin/radar.exe
PDB_TARGET=bin/radar.pdb

$(SFMT_TARGET):
	@$(CC) -O2 -Oi -DHAVE_SSE2=1 -DSFMT_MEXP=19937 -c ext/sfmt/SFMT.c -Fo$(SFMT_OBJECT)
	@$(STATICLIB) $(SFMT_OBJECT) -OUT:$(SFMT_TARGET)
	@rm $(SFMT_OBJECT)

$(OBJ_DIR)%.obj: $(SRC_DIR)%.cpp
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -Fo$@

$(OBJ_DIR)Game/%.obj: $(SRC_DIR)Game/%.cpp
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -Fo$@

$(OBJ_DIR)Systems/%.obj: $(SRC_DIR)Systems/%.cpp
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -Fo$@

radar: $(SFMT_TARGET) $(OBJS)
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC $(OBJS) $(LINK) $(LIB_FLAGS) /OUT:$(TARGET) /PDB:$(PDB_TARGET)

##################################################
# NOTE - LINUX BUILD
##################################################
else # GCC
OPENAL_LIB=ext/openal-soft/build
GLFW_LIB=ext/glfw/build/src
SFMT_OBJECT=ext/sfmt/SFMT.o
SFMT_TARGET=ext/sfmt/libSFMT.a

OBJS=$(patsubst %.cpp,$(OBJ_DIR)%.o,$(SRCS))

CC=g++
CFLAGS=-Wno-unused-variable -Wno-unused-parameter -Wno-write-strings -D_CRT_SECURE_NO_WARNINGS -DSFMT_MEXP=19937 -std=c++11 -pedantic
DEBUG_FLAGS=-g -DDEBUG -Wall -Wextra
RELEASE_FLAGS=-O2
VERSION_FLAGS=$(DEBUG_FLAGS)

LIB_FLAGS=-Lext/ -L$(RF_DIR)ext/ -L$(OPENAL_LIB) -L$(GLEW_LIB) -L$(GLFW_LIB) -L$(CJSON_LIB) -L$(SFMT_LIB) -L$(RF_LIB) -lrf -lSFMT -lstb -lcJSON -lglfw -lglew -lopenal \
		  -lGL -lX11 -lXinerama -lXrandr -lXcursor -lm -ldl -lpthread

TARGET=bin/radar

# Compile with specific flags for this one
$(SFMT_TARGET):
	@echo "AR $(SFMT_TARGET)"
	@$(CC) -O3 -msse2 -fno-strict-aliasing -DHAVE_SSE2=1 -DSFMT_MEXP=19937 -c ext/sfmt/SFMT.c -o $(SFMT_OBJECT)
	@ar rcs $(SFMT_TARGET) $(SFMT_OBJECT)
	@rm $(SFMT_OBJECT)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@echo "CC $@"
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -o $@

$(OBJ_DIR)Game/%.o: $(SRC_DIR)Game/%.cpp
	@echo "CC $@"
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -o $@

$(OBJ_DIR)Systems/%.o: $(SRC_DIR)Systems/%.cpp
	@echo "CC $@"
	@$(CC) $(CFLAGS) $(VERSION_FLAGS) $(INCLUDE_FLAGS) -DGLEW_STATIC -c $< -o $@

radar: $(SFMT_TARGET) $(OBJS)
	@echo "CC $(TARGET)"
	$(CC) $(CFLAGS) $(VERSION_FLAGS) -DGLEW_STATIC $(OBJS) $(INCLUDE_FLAGS) $(LIB_FLAGS) -o $(TARGET)

endif
#$(error OS not compatible. Only Win32 and Linux for now.)

##################################################
##################################################

pre_build:
	@mkdir -p bin
	@mkdir -p obj/Game
	@mkdir -p obj/Systems

post_build:
	@rm -f *.lib *.exp

clean:
	rm $(OBJS)
	rm $(TARGET)

clean_ext:
	rm $(SFMT_TARGET)

tags:
	@ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRC_DIR)*.cpp $(SRC_DIR)*.h $(LIB_INCLUDES) $(RF_INCLUDE)/rf/*.h
