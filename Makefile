LIBS=-lm -lrt

all: hoh

hoh: hoh.cpp platform.h rans64.h channel.h file_io.h symbolstats.h patches.h lz.h
	g++ -o $@ $< -O3 $(LIBS)
