LIBS=-lm -lrt

all: choh dhoh simple_entropy_encoder simple_entropy_decoder tests

choh: choh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp lz.hpp bitimage.hpp entropy_encoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

dhoh: dhoh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp entropy_decoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

simple_entropy_encoder: simple_entropy_encoder.cpp file_io.hpp rans64.hpp entropy_encoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

simple_entropy_decoder: simple_entropy_decoder.cpp file_io.hpp rans64.hpp entropy_decoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

tests: pre_tests entropy_roundtrip rgb_roundtrip

pre_tests:
	@echo "---TESTS---"

entropy_roundtrip: entropy_roundtrip_test.sh simple_entropy_encoder simple_entropy_decoder
	@./entropy_roundtrip_test.sh
	@echo ""

rgb_roundtrip: rgb_roundtrip_test.sh choh dhoh
	@./rgb_roundtrip_test.sh
	@echo ""
