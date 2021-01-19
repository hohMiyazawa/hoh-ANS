LIBS=-lm -lrt

all: choh dhoh simple_entropy_encoder simple_entropy_decoder

choh: choh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp lz.hpp bitimage.hpp entropy_encoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

dhoh: dhoh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp entropy_decoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

simple_entropy_encoder: simple_entropy_encoder.cpp file_io.hpp rans64.hpp entropy_encoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

simple_entropy_decoder: simple_entropy_decoder.cpp file_io.hpp rans64.hpp entropy_decoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)
