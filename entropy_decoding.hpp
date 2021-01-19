#ifndef ENTROPY_DECODING_HEADER
#define ENTROPY_DECODING_HEADER

#include "rans64.hpp"
#include "varint.hpp"
#include "stattools.hpp"

uint16_t* decode_entropy(
	uint8_t* in_bytes,
	size_t in_size,
	size_t* byte_pointer,
	size_t* symbol_size
){
	size_t symbol_range = read_varint(in_bytes, byte_pointer)+1;
	*symbol_size = read_varint(in_bytes, byte_pointer);

	uint8_t maximum_bits_per_symbol = 0;
	for(size_t check = (symbol_range - 1);check;check = (check>>1)){
		maximum_bits_per_symbol++;
	}

	uint8_t metadata = in_bytes[(*byte_pointer)++];
	uint8_t entropy_mode = metadata>>7;
	uint8_t prob_bits = (metadata & 0b00111100)>>2;
	uint8_t table_storage_mode = (metadata & 0b00000011);

	uint16_t* decoded = new uint16_t[*symbol_size];

//diagnostics
///*
	printf("entropy_mode       : %d\n",(int)entropy_mode);
	printf("prob_bits          : %d\n",(int)prob_bits);
	printf("table_storage_mode : %d\n",(int)table_storage_mode);
	printf("range              : %d\n",(int)symbol_range);
	printf("bits per symbol    : %d\n",(int)maximum_bits_per_symbol);
	printf("symbols            : %d\n",(int)(*symbol_size));
//*/

	if(entropy_mode){
		uint32_t freqs[symbol_range];
		uint32_t cum_freqs[symbol_range + 1];
		Rans64DecSymbol dsyms[symbol_range];
		uint8_t slag = 0;
		uint8_t slag_bits = 0;
		if(table_storage_mode == 0){
			for(int i=0;i<symbol_range;i++){
				freqs[i] = 1;
			}
			normalize_freqs(freqs,cum_freqs,symbol_range,1<<prob_bits);
		}
		else if(table_storage_mode == 1){
			//raw values for freqs
			for(int i=0;i<symbol_range;i++){
				freqs[i] = unstuffer(
					in_bytes,
					byte_pointer,
					&slag,
					&slag_bits,
					maximum_bits_per_symbol
				);
			}
			/*for(int i=0;i<symbol_range;i++){
				printf("%d\n",(int)freqs[i]);
			}*/
			calc_cum_freqs(freqs,cum_freqs, symbol_range);
		}
		else if(table_storage_mode == 2){
			int clamp_number = ((prob_bits - 1)/4 + 2);
			uint32_t lower_clamps[clamp_number];
			uint32_t upper_clamps[clamp_number];
			for(int i=0;i<clamp_number;i++){
				lower_clamps[i] = unstuffer(
					in_bytes,
					byte_pointer,
					&slag,
					&slag_bits,
					maximum_bits_per_symbol
				);
				upper_clamps[i] = unstuffer(
					in_bytes,
					byte_pointer,
					&slag,
					&slag_bits,
					maximum_bits_per_symbol
				);
				printf("  clamps: %d %d\n",(int)lower_clamps[i],(int)upper_clamps[i]);
			}
			for(int i=0;i<symbol_range;i++){
				uint8_t symbol_bits = 0;
				if(lower_clamps[0] <= i && upper_clamps[0] >= i){
					symbol_bits = 1;
				}
				if(lower_clamps[1] <= i && upper_clamps[1] >= i){
					symbol_bits = 4;
				}
				for(int j=2;j<clamp_number;j++){
					if(lower_clamps[j] <= i && upper_clamps[j] >= i){
						symbol_bits = 4*j;
					}
				}
				if(symbol_bits > prob_bits){
					symbol_bits = prob_bits;
				}
				freqs[i] = unstuffer(
					in_bytes,
					byte_pointer,
					&slag,
					&slag_bits,
					symbol_bits
				);
			}
		}
		else if(table_storage_mode == 3){
			printf("unimplemented frequency table storage mode!\n");
		}
		else{
			printf("unknown frequency table storage mode!\n");
		}
		size_t data_size = read_varint(in_bytes, byte_pointer);
		printf("---rANS size: %d\n",(int)data_size);
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

uint8_t* decode_entropy_8bit(//make dedicated 8bit version later
	uint8_t* in_bytes,
	size_t in_size,
	size_t* byte_pointer,
	size_t* symbol_size
){
	uint16_t* decoded = decode_entropy(
		in_bytes,
		in_size,
		byte_pointer,
		symbol_size
	);
	uint8_t* decoded2 = new uint8_t[*symbol_size];
	for(size_t i=0;i<(*symbol_size);i++){
		decoded2[i] = (uint8_t)decoded[i];
	}
	delete[] decoded;
	return decoded2;
}

#endif //ENTROPY_DECODING_HEADER
