#ifndef ENTROPY_DECODING_HEADER
#define ENTROPY_DECODING_HEADER

#include "rans64.hpp"
#include "varint.hpp"

uint16_t* decode_entropy(
	uint8_t* in_bytes,
	size_t in_size,
	size_t* byte_pointer,
	size_t* symbol_size
){
	size_t symbol_range = read_varint(in_bytes, byte_pointer)+1;
	*symbol_size = read_varint(in_bytes, byte_pointer);

	uint8_t metadata = in_bytes[(*byte_pointer)++];
	uint8_t entropy_mode = metadata>>7;
	uint8_t prob_bits = (metadata & 0b00111100)>>2;
	uint8_t table_storage_mode = (metadata & 0b00000011);

	uint16_t* decoded = new uint16_t[*symbol_size];

	if(entropy_mode){
		uint32_t freqs[symbol_range];
		uint32_t cum_freqs[symbol_range + 1];
		Rans64DecSymbol dsyms[symbol_range];

		if(table_storage_mode == 0){
			for(int i=0;i<symbol_range;i++){
				freqs[i] = 1;
			}
		}
		else if(table_storage_mode == 1){
		}
		else if(table_storage_mode == 2){
		}
		else if(table_storage_mode == 3){
		}
		else{
			printf("unknown frequency table storage mode!\n");
		}
		size_t data_size = read_varint(in_bytes, byte_pointer);
		for(int i=0; i < symbol_range; i++) {
			Rans64DecSymbolInit(&dsyms[i], cum_freqs[i], freqs[i]);
		}
		uint16_t cum2sym[1<<prob_bits];
		for(int s=0; s < symbol_range; s++){
			for(uint32_t i=cum_freqs[s]; i < cum_freqs[s+1]; i++){
		   		 cum2sym[i] = s;
			}
		}

		Rans64State rans;
		uint32_t* ptr = (uint32_t*)(in_bytes + *byte_pointer);
		Rans64DecInit(&rans, &ptr);

		for(size_t i=0; i < *symbol_size; i++) {
			uint32_t s = cum2sym[Rans64DecGet(&rans, prob_bits)];
			decoded[i] = (uint16_t) s;
			Rans64DecAdvanceSymbol(&rans, &ptr, &dsyms[s], prob_bits);
		}
	}
	else{
		//only 8bit for now
		for(size_t i=0;i<*symbol_size;i++){
			decoded[i] = (uint16_t)in_bytes[(*byte_pointer)++];
		}
	}
	return decoded;
}

#endif //ENTROPY_DECODING_HEADER
