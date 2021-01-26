#ifndef ENTROPY_ENCODING_HEADER
#define ENTROPY_ENCODING_HEADER

#include "rans64.hpp"
#include "varint.hpp"
#include "stattools.hpp"

size_t encode_entropy(
	uint16_t* symbols,
	size_t symbol_size,
	size_t range,
	uint8_t* output_bytes,
	uint32_t prob_bits,
	uint8_t diagnostics
){
	uint8_t entropy_mode;
	uint8_t table_storage_mode = 0;
	size_t entropy_size = 0;
	if(symbol_size == 0){
		write_varint(output_bytes, &entropy_size, range-1);
		write_varint(output_bytes, &entropy_size, symbol_size);
		return entropy_size;
	}
	uint8_t maximum_bits_per_symbol = 0;
	for(size_t check = (range - 1);check;check = (check>>1)){
		maximum_bits_per_symbol++;
	}

	//printf("maximum_bits_per_symbol: %d\n",(int)maximum_bits_per_symbol);

	uint32_t prob_scale = 1 << prob_bits;
	uint32_t* freqs     = new uint32_t[range];
	for(size_t i=0;i<range;i++){
		freqs[i] = 0;
	}
	uint32_t* cum_freqs = new uint32_t[range + 1];
	for(size_t i=0;i<symbol_size;i++){
		freqs[symbols[i]]++;
	}
	//printf("prob_scale: %d\n",(int)prob_scale);
	normalize_freqs(freqs,cum_freqs,range,prob_scale);
//encode table here
	write_varint(output_bytes, &entropy_size, range-1);
	write_varint(output_bytes, &entropy_size, symbol_size);
	size_t expected_stored_size = entropy_size/*header already written*/ + 1/*to store or not to store*/ + (maximum_bits_per_symbol*symbol_size + 8 - 1)/8/*the data*/;

	size_t expected_raw_size = (prob_bits*range + 8 - 1)/8;
	size_t expected_clamped_size = 2*(maximum_bits_per_symbol - 1)*((prob_bits - 1)/4 + 2);
	expected_clamped_size += prob_bits*2;

	uint8_t clamp_number = ((prob_bits - 1)/4 + 2);

	uint16_t lower_clamps[clamp_number];
	uint16_t upper_clamps[clamp_number];

	size_t size_bits = 0;
	size_t climb = 0;
	size_t lower_clamp_index = 0;
	for(;climb<range;climb++){
		while(freqs[climb] >= (size_t)(1<<size_bits)){
			if(size_bits == 0){
				size_bits = 1;
				lower_clamps[0] = climb;
				lower_clamp_index = 1;
			}
			else if(size_bits == 1){
				size_bits = 4;
				lower_clamps[1] = climb;
				lower_clamp_index = 2;
			}
			else{
				lower_clamps[size_bits/4 + 1] = climb;
				size_bits += 4;
				lower_clamp_index++;
			}
		}
		if(size_bits >= prob_bits){
			size_bits = prob_bits;
			expected_clamped_size += size_bits;
			break;
		}
		expected_clamped_size += size_bits;
	}
	while(lower_clamp_index < clamp_number){
		lower_clamps[lower_clamp_index++] = range - 1;
	}
	size_bits = 0;
	size_t climb2 = range - 1;
	size_t upper_clamp_index = 0;
	for(;climb2 >= 0;climb2--){
		while(freqs[climb2] >= (size_t)(1<<size_bits)){
			if(size_bits == 0){
				size_bits = 1;
				upper_clamps[0] = climb2;
				upper_clamp_index = 1;
			}
			else if(size_bits == 1){
				size_bits = 4;
				upper_clamps[1] = climb2;
				upper_clamp_index = 2;
			}
			else{
				upper_clamps[size_bits/4 + 1] = climb2;
				size_bits += 4;
				upper_clamp_index++;
			}
		}
		if(size_bits >= prob_bits){
			size_bits = prob_bits;
			expected_clamped_size += size_bits;
			break;
		}
		expected_clamped_size += size_bits;
		if(climb2 == 0){
			break;
		}
	}
	while(upper_clamp_index < clamp_number){
		upper_clamps[upper_clamp_index++] = 0;
	}
	expected_clamped_size += size_bits*(climb2 - climb - 1);
	expected_clamped_size = (expected_clamped_size + 8 - 1)/8;
	//printf("tab %d %d\n",(int)expected_raw_size,(int)expected_clamped_size);

	if(diagnostics){
		printf("  range: %d\n",(int)range);
		for(size_t i=0;i<clamp_number;i++){
			printf("  clamps: %d %d\n",(int)lower_clamps[i],(int)upper_clamps[i]);
		}
	}

	if(diagnostics){
		printf("entropy_size %d\n",(int)entropy_size);
	}
	if(0/*remove when decoder ready*/ || (expected_raw_size < expected_clamped_size)){
		entropy_mode = 1;
		table_storage_mode = 1;
		output_bytes[entropy_size++] = (entropy_mode<<7) + (prob_bits<<2) + table_storage_mode;
		uint8_t bits_remaining = 8;
		uint8_t current_byte = 0;
		for(size_t i=0;i<range;i++){
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[i],maximum_bits_per_symbol);
		}
		if(bits_remaining != 8){
			output_bytes[entropy_size++] = current_byte;
		}
	}
	else{
		entropy_mode = 1;
		table_storage_mode = 2;
		output_bytes[entropy_size++] = (entropy_mode<<7) + (prob_bits<<2) + table_storage_mode;
		uint8_t bits_remaining = 8;
		uint8_t current_byte = 0;
		for(size_t i=0;i<clamp_number;i++){
			stuffer(
				output_bytes,
				&entropy_size,
				&current_byte,
				&bits_remaining,
				(uint32_t)lower_clamps[i],
				maximum_bits_per_symbol
			);
			stuffer(
				output_bytes,
				&entropy_size,
				&current_byte,
				&bits_remaining,
				(uint32_t)upper_clamps[i],
				maximum_bits_per_symbol
			);
		}
		for(size_t i=0;i<range;i++){
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
			stuffer(
				output_bytes,
				&entropy_size,
				&current_byte,
				&bits_remaining,
				freqs[i],
				symbol_bits
			);
		}
		if(bits_remaining != 8){
			output_bytes[entropy_size++] = current_byte;
		}
	}
	if(diagnostics){
		printf("entropy_size %d\n",(int)entropy_size);
	}
//end encode table

	static size_t out_max_size = 32<<20; // 32MB
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	uint32_t* out_buf = new uint32_t[out_max_elems];
	uint32_t* out_end = out_buf + out_max_elems;

	Rans64EncSymbol esyms[range];

	for(size_t i=0; i < range; i++){
		//printf("        esyms %d %d\n",(int)cum_freqs[i], (int)freqs[i]);
		Rans64EncSymbolInit(&esyms[i], cum_freqs[i], freqs[i], prob_bits);
	}

	Rans64State rans;
	Rans64EncInit(&rans);

	uint32_t* ptr = out_end; // *end* of output buffer
	for (size_t i=symbol_size; i > 0; i--) { // NB: working in reverse!
		int s = symbols[i-1];
		Rans64EncPutSymbol(&rans, &ptr, &esyms[s], prob_bits);
	}
	Rans64EncFlush(&rans, &ptr);
	uint32_t* rans_begin = ptr;

	if(diagnostics){
		printf("---rANS size: %d\n",(int)((out_end - rans_begin)*4));
	}
	write_varint(output_bytes, &entropy_size, (out_end - rans_begin)*4);

	while(rans_begin < out_end){
		*((uint32_t*)(output_bytes + entropy_size)) = *rans_begin;
		rans_begin++;
		entropy_size += 4;
	}

	delete[] freqs;
	delete[] cum_freqs;
	delete[] out_buf;

	if(expected_stored_size < entropy_size){
		entropy_mode = 0;
		entropy_size = 0;
		table_storage_mode = 0;
		prob_bits = 0;
		write_varint(output_bytes, &entropy_size, range-1);
		write_varint(output_bytes, &entropy_size, symbol_size);
		output_bytes[entropy_size++] = 0;
		uint8_t bits_remaining = 8;
		uint8_t current_byte = 0;
		for(size_t i=0;i<symbol_size;i++){
			stuffer(
				output_bytes,
				&entropy_size,
				&current_byte,
				&bits_remaining,
				symbols[i],
				maximum_bits_per_symbol
			);
		}
		if(bits_remaining != 8){
			output_bytes[entropy_size++] = current_byte;
		}
	}

	if(diagnostics){
		printf("entropy_mode       : %d\n",(int)entropy_mode);
		printf("prob_bits          : %d\n",(int)prob_bits);
		printf("table_storage_mode : %d\n",(int)table_storage_mode);
		printf("range              : %d\n",(int)range);
		printf("bits per symbol    : %d\n",(int)maximum_bits_per_symbol);
		printf("symbols            : %d\n",(int)symbol_size);
	}
	
	//printf("entropy: %d\n",(int)entropy_size);
	return entropy_size;
}

//make a dedicated 8bit version later
size_t encode_entropy(
	uint8_t* symbols,
	size_t symbol_size,
	size_t range,
	uint8_t* output_bytes,
	uint32_t prob_bits,
	uint8_t diagnostics
){
	uint16_t symbols2[symbol_size];
	for(size_t i=0;i<symbol_size;i++){
		symbols2[i] = (uint16_t)symbols[i];
	}
	return encode_entropy(
		symbols2,
		symbol_size,
		range,
		output_bytes,
		prob_bits,
		diagnostics
	);
}

#endif //ENTROPY_ENCODING_HEADER
