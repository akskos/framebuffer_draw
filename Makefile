all:
	mkdir -p bin
	gcc -std=c99 -o ./bin/fb *.c -lpng12
