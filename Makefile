LIBS=-lm -lrt

all: choh dhoh

choh: choh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp lz.hpp bitimage.hpp entropy_encoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

dhoh: dhoh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp entropy_decoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)
