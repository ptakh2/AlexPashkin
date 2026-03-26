all: main.o lodepng.o
		gcc main.o lodepng.o -o ships.exe

main.o: main.c
		gcc main.c -c

lodepng.o: lodepng.c
		gcc lodepng.c -c

debug: main-debug.o lodepng-debug.o
		gcc main.c -g -c -o main-debug.o

main-debug.o: main.c
		gcc main.c -g -c -o main-debug.o

lodepng-debug.o: lodepng.c
		gcc lodepng.c -g -c -o lodepng-debug.o

clean:
		rm *.o
		rm ships.exe