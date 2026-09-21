CXX = g++
CFLAGS = -Wall -g -fno-omit-frame-pointer
#CFLAGS = -Wall -g -O3 -fno-omit-frame-pointer -march=native -ffast-math
CDEFS =
LDLIBS = -lm

stamp-compress-2.x : stamp-compress-2.cpp
	$(CXX) $(CFLAGS) $(CDEFS) stamp-compress-2.cpp $(LDLIBS) -o stamp-compress-2.x
