
run: build/racing
	LD_LIBRARY_PATH="build/sdl" build/racing

d: build/racing
	LD_LIBRARY_PATH="build/sdl" gf2 build/racing

build/racing: build/main.o build/sdl/libSDL3.so.0.1.2
	clang -o build/racing build/main.o build/sdl/libSDL3_test.a -lSDL3 -lm

build/main.o: main.c
	clang -ggdb -I./SDL/include -o build/main.o -c main.c

