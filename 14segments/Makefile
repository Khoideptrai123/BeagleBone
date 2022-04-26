all:
	arm-linux-gnueabihf-gcc -g -std=c99 -D  _POSIX_C_SOURCE=200809L -Werror 14segment.c segment.c potentiometer.c circularbuffer.c sampler.c -o testBuffer -pthread
	cp testBuffer $(HOME)/cmpt433/public/myApps/