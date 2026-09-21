CXX = gcc
CFLAGS = -Wall -g -fno-omit-frame-pointer
#CFLAGS = -Wall -g -O3 -fno-omit-frame-pointer -march=native -ffast-math
CDEFS =
LDLIBS = -lm

oprend.x : oprend.c
	$(CXX) $(CFLAGS) $(CDEFS) oprend.c $(LDLIBS) -o oprend.x
