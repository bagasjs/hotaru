CC=clang
CFLAGS=-Wall -Wextra -Ivendors/glad/include -Ivendors/stb/include -ggdb
LFLAGS=`pkg-config --libs glfw3` -lGL -lm

all: build build/main

build/main: ./main.c ./hotaru.c ./vendors/glad/src/glad.c
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

build:
	@mkdir $@

