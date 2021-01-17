#ifndef ENTROPY_ENCODING_HEADER
#define ENTROPY_ENCODING_HEADER

#include "rans64.hpp"
#include "varint.hpp"

size_t encode_entropy(
	uint16_t* symbols,
	size_t symbol_size,
	size_t range,
	uint8_t* output_bytes,
	uint8_t effort
){
	uint8_t maximum_bits_per_symbol = 0;
	for(size_t check = (range - 1);check;check = (check>>1)){
		maximum_bits_per_symbol++;
	}
	size_t maximum_size = 10 + (symbol_size*maximum_bits_per_symbol + 8 - 1)/8;
	size_t decode_size = 0;
	output_bytes = new uint8_t[maximum_size];
	if(effort == 0){
		if(maximum_size == 8){
			write_varint(output_bytes, &decode_size, range-1);
			write_varint(output_bytes, &decode_size, symbol_size-1);
			output_bytes[decode_size++] = 0;
			for(size_t i = 0;i<symbol_size;i++){
				output_bytes[decode_size++] = symbols[i];
			}
		}
		else{
			printf("unimplemented storage mode\n");
		}
	}
	return decode_size;
}

#endif //ENTROPY_ENCODING_HEADER
