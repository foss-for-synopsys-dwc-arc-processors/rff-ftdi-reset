CFLAGS=-O0 -Wall -Wextra

LIBS=$(shell pkg-config --libs libftdi1)
LIBS+=$(shell pkg-config --libs libusb-1.0)

CFLAGS=$(shell pkg-config --cflags libftdi1)
CFLAGS+=$(shell pkg-config --cflags libusb-1.0)

rff-reset: rff-reset.o
	gcc rff-reset.c -o rff-reset ${CFLAGS} ${LIBS}

all: rff-reset

clean:
	rm -f rff-reset
	rm -f rff-reset.o
