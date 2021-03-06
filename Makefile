LIBS=-lm -lrt

all: choh dhoh simple_entropy_encoder simple_entropy_decoder simple_parser tests

choh: choh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp lz.hpp bitimage.hpp entropy_encoding.hpp varint.hpp stattools.hpp layer_encode.hpp prediction.hpp predictor_operations.hpp
	g++ -o $@ $< -O3 $(LIBS) -Wall

dhoh: dhoh.cpp platform.hpp rans64.hpp channel.hpp file_io.hpp entropy_decoding.hpp varint.hpp stattools.hpp layer_decode.hpp unprediction.hpp predictor_operations.hpp un_lz.hpp
	g++ -o $@ $< -O3 $(LIBS) -Wall

simple_entropy_encoder: simple_entropy_encoder.cpp file_io.hpp rans64.hpp entropy_encoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

simple_entropy_decoder: simple_entropy_decoder.cpp file_io.hpp rans64.hpp entropy_decoding.hpp varint.hpp stattools.hpp
	g++ -o $@ $< -O3 $(LIBS)

simple_parser: simple_parser.cpp file_io.hpp varint.hpp layer_decode.hpp un_lz.hpp entropy_decoding.hpp
	g++ -o $@ $< -O3 $(LIBS)

layer_roundtrip: layer_roundtrip_test.cpp rans64.hpp entropy_encoding.hpp entropy_decoding.hpp varint.hpp stattools.hpp layer_encode.hpp layer_decode.hpp unprediction.hpp prediction.hpp predictor_operations.hpp
	@g++ -o $@ $< -O3 $(LIBS)

tests: pre_tests entropy_roundtrip_test layer_roundtrip_test rgb_roundtrip_test

pre_tests:
	@echo "---TESTS---"

entropy_roundtrip_test: entropy_roundtrip_test.sh simple_entropy_encoder simple_entropy_decoder
	@./entropy_roundtrip_test.sh
	@echo ""

layer_roundtrip_test: layer_roundtrip
	@./layer_roundtrip
	@echo ""

rgb_roundtrip_test: rgb_roundtrip_test.sh choh dhoh
	@./rgb_roundtrip_test.sh
	@echo ""
