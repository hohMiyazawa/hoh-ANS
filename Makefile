LIBS=-lm -lrt

all: hoh

hoh: hoh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp symbolstats.hpp lz.hpp channel_encode.hpp bitimage.hpp
	g++ -o $@ $< -O3 $(LIBS)
