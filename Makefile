LIBS=-lm -lrt

all: choh dhoh

choh: choh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp symbolstats.hpp lz.hpp channel_encode.hpp bitimage.hpp
	g++ -o $@ $< -O3 $(LIBS)

dhoh: dhoh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp symbolstats.hpp lz.hpp channel_encode.hpp bitimage.hpp
	g++ -o $@ $< -O3 $(LIBS)
