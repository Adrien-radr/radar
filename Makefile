# Prerequisites
#STATIC_GLFW_LIB = /home/adrien/Documents/perso/glfw-3.1.1/src/libglfw3.a
GLFW_INCLUDE = $(shell pkg-config --cflags glfw3)
GLFW_LIBS = $(shell pkg-config --static --libs glfw3)
ASSIMP_INCLUDE = $(shell pkg-config --cflags assimp)
ASSIMP_LIBS = $(shell pkg-config --libs assimp)
FREETYPE_INCLUDE = $(shell pkg-config --cflags freetype2)
FREETYPE_LIBS = $(shell pkg-config --libs freetype2)
PNG_INCLUDE = $(shell pkg-config --cflags libpng)
PNG_LIBS = $(shell pkg-config --libs libpng)
ZLIB_INCLUDE = $(shell pkg-config --cflags zlib)
ZLIB_LIBS = $(shell pkg-config --libs zlib)
IMGUI_INCLUDE = -Iext/imgui

# Config
COMMON_FLAGS = -Iradar/ -Iradar/src -Iradar/ext $(IMGUI_INCLUDE) $(GLFW_INCLUDE) $(ASSIMP_INCLUDE) $(FREETYPE_INCLUDE) $(PNG_INCLUDE) $(ZLIB_INCLUDE) -DPNG_SKIP_SETJMP_CHECK -std=c++11 $(OPTFLAGS)
DEBUG_FLAGS = -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -D_DEBUG $(COMMON_FLAGS)
RELEASE_FLAGS = -O2 -D_NDEBUG $(COMMON_FLAGS)

CC = g++
PREFIX ?= /usr/local
LIBS = $(GLFW_LIBS) -lGL $(ASSIMP_LIBS) $(FREETYPE_LIBS) $(PNG_LIBS) $(ZLIB_LIBS) -lm $(OPTLIBS)


# Config Sample
TARGET = bin/sun
CFLAGS = $(DEBUG_FLAGS)

TEST_SRC = $(wildcard tests/*_tests.c)
TESTS = $(patsubst %.c,%,$(TEST_SRC))

SRCS = $(wildcard ./*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))

.PHONY: sun, depend, tests, release, clean, install, check, external

all: sun

release: LIB_CFLAGS = $(RELEASE_FLAGS)
release: all

depend:
	makedepend -- $(LIB_CFLAGS) -- $(LIB_SOURCES)

lib:
	$(MAKE) -C radar/

#------------------------------------------------------------------
sun: lib
	@echo "CC		$(TARGET)"
	@$(CC) $(CFLAGS) $(SRCS) -Lradar/bin/ -lradar $(LIBS) -o $(TARGET)

clean:
	rm $(OBJS)

cleanall:
	$(MAKE) -C radar/ clean
	rm -rf build

install: all
	@echo "Nothing set to be installed!"
