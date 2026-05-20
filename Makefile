# AMPL user-defined functions as a shared library (see funclink README).
# https://www.netlib.org/ampl/solvers/funclink/README

CC ?= cc
CFLAGS ?= -O2
CPPFLAGS += -I.

.PHONY: all clean

all: amplfunc.dll

amplfunc.dll: amplfunc.c funcadd.h stdio1.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared -o amplfunc.dll amplfunc.c -lm

clean:
	rm -f amplfunc.dll

