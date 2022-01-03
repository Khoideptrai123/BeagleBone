all:
	arm-linux-gnueabihf-gcc -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror 14segment.c -o 14segment
	cp 14segment $(HOME)/cmpt433/public/myApps/