#ifndef ENTROPY_ENCODING_HEADER
#define ENTROPY_ENCODING_HEADER

#include <assert.h>

#include "rans64.hpp"
#include "varint.hpp"

void calc_cum_freqs(uint32_t* freqs,uint32_t* cum_freqs, size_t size){
	cum_freqs[0] = 0;
	for (size_t i=0; i < size; i++){
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
	}
}

void normalize_freqs(uint32_t* freqs,uint32_t* cum_freqs, size_t size,uint32_t target_total){
	assert(target_total >= size);
	
	calc_cum_freqs(freqs,cum_freqs,size);

	uint32_t cur_total = cum_freqs[size];
	
	// resample distribution based on cumulative freqs
	for (int i = 1; i <= size; i++)
		cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;

	// if we nuked any non-0 frequency symbol to 0, we need to steal
	// the range to make the frequency nonzero from elsewhere.
	//
	// this is not at all optimal, i'm just doing the first thing that comes to mind.
	for (int i=0; i < size; i++) {
		if (freqs[i] && cum_freqs[i+1] == cum_freqs[i]) {
			// symbol i was set to zero freq

			// find best symbol to steal frequency from (try to steal from low-freq ones)
			uint32_t best_freq = ~0u;
			int best_steal = -1;
			for (int j=0; j < size; j++) {
				uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
				if (freq > 1 && freq < best_freq) {
					best_freq = freq;
					best_steal = j;
				}
			}
			assert(best_steal != -1);

			// and steal from it!
			if (best_steal < i) {
				for (int j = best_steal + 1; j <= i; j++)
					cum_freqs[j]--;
			} else {
				assert(best_steal > i);
				for (int j = i + 1; j <= best_steal; j++)
					cum_freqs[j]++;
			}
		}
	}

	// calculate updated freqs and make sure we didn't screw anything up
	assert(cum_freqs[0] == 0 && cum_freqs[size] == target_total);
	for (int i=0; i < size; i++) {
		if (freqs[i] == 0)
			assert(cum_freqs[i+1] == cum_freqs[i]);
		else
			assert(cum_freqs[i+1] > cum_freqs[i]);

		// calc updated freq
		freqs[i] = cum_freqs[i+1] - cum_freqs[i];
	}
}

size_t encode_entropy(
	uint16_t* symbols,
	size_t symbol_size,
	size_t range,
	uint8_t* output_bytes,
	uint32_t prob_bits
){
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
	size_t expected_stored_size = entropy_size + 1 + (maximum_bits_per_symbol*symbol_size + 8 - 1)/8;

	size_t expected_raw_size = (prob_bits*range + 8 - 1)/8;
	size_t expected_clamped_size = 2*(maximum_bits_per_symbol - 1)*((prob_bits - 1)/4 + 2);
	expected_clamped_size += prob_bits*2;

	uint16_t lower_clamps[((prob_bits - 1)/4 + 2)];
	uint16_t upper_clamps[((prob_bits - 1)/4 + 2)];

	int size_bits = 0;
	for(int i=0;i<range/2 - 1;i++){
		while(freqs[i] >= (1<<size_bits)){
			if(size_bits == 0){
				size_bits = 1;
				lower_clamps[0] = i;
			}
			else if(size_bits == 1){
				size_bits = 4;
				lower_clamps[1] = i;
			}
			else{
				lower_clamps[size_bits/4 + 1] = i;
				size_bits += 4;
			}
		}
		if(size_bits > prob_bits){
			size_bits = prob_bits;
		}
		expected_clamped_size += size_bits;
	}
	size_bits = 0;
	for(int i=range - 1;i>range/2;i--){
		while(freqs[i] >= (1<<size_bits)){
			if(size_bits == 0){
				size_bits = 1;
				upper_clamps[0] = 511 - i;
			}
			else if(size_bits == 1){
				size_bits = 4;
				upper_clamps[1] = 511 - i;
			}
			else{
				upper_clamps[size_bits/4 + 1] = 511 - i;
				size_bits += 4;
			}
		}
		if(size_bits > prob_bits){
			size_bits = prob_bits;
		}
		expected_clamped_size += size_bits;
	}
	expected_clamped_size = (expected_clamped_size + 8 - 1)/8;
	//printf("tab %d %d\n",(int)expected_raw_size,(int)expected_clamped_size);
	if(expected_raw_size < expected_clamped_size){
		output_bytes[entropy_size++] = (1<<7) + (prob_bits<<2) + 1;
		uint8_t bits_remaining = 8;
		uint8_t current_byte;
		for(int i=0;i<range;i++){
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[i],maximum_bits_per_symbol);
		}
		if(bits_remaining != 8){
			output_bytes[entropy_size++] = current_byte;
		}
	}
	else{
		output_bytes[entropy_size++] = (1<<7) + (prob_bits<<2) + 2;
		uint8_t bits_remaining = 8;
		uint8_t current_byte;
		for(int i=0;i<((prob_bits - 1)/4 + 2);i++){
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)lower_clamps[i],maximum_bits_per_symbol - 1);
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)upper_clamps[i],maximum_bits_per_symbol - 1);
		}
		int size_bits = 0;
		for(int i=0;i<range/2 - 1;i++){
			while(freqs[i] >= (1<<size_bits)){
				if(size_bits == 0){
					size_bits = 1;
					lower_clamps[0] = i;
				}
				else if(size_bits == 1){
					size_bits = 4;
					lower_clamps[1] = i;
				}
				else{
					lower_clamps[size_bits/4 + 1] = i;
					size_bits += 4;
				}
			}
			if(size_bits > prob_bits){
				size_bits = prob_bits;
			}
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[i],size_bits);
		}
		stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[range/2 - 1],maximum_bits_per_symbol);
		size_bits = 0;
		for(int i=range - 1;i>range/2;i--){
			while(freqs[i] >= (1<<size_bits)){
				if(size_bits == 0){
					size_bits = 1;
					upper_clamps[0] = 511 - i;
				}
				else if(size_bits == 1){
					size_bits = 4;
					upper_clamps[1] = 511 - i;
				}
				else{
					upper_clamps[size_bits/4 + 1] = 511 - i;
					size_bits += 4;
				}
			}
			if(size_bits > prob_bits){
				size_bits = prob_bits;
			}
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[i],size_bits);
		}
		stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[range/2],maximum_bits_per_symbol);
		if(bits_remaining != 8){
			output_bytes[entropy_size++] = current_byte;
		}
	}

//end encode table
	static size_t out_max_size = 32<<20; // 32MB
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	uint32_t* out_buf = new uint32_t[out_max_elems];
	uint32_t* out_end = out_buf + out_max_elems;

	Rans64EncSymbol esyms[range];

	for (int i=0; i < range; i++) {
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

	while(rans_begin < out_end){
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>24);
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>16);
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>8);
		output_bytes[entropy_size++] = (uint8_t)(*rans_begin);
		rans_begin++;
	}

	delete[] freqs;
	delete[] cum_freqs;
	delete[] out_buf;

	if(expected_stored_size < entropy_size){
		entropy_size = 0;
		write_varint(output_bytes, &entropy_size, range-1);
		write_varint(output_bytes, &entropy_size, symbol_size);
		output_bytes[entropy_size++] = 0;
	}
	
	//printf("entropy: %d\n",(int)entropy_size);
	return entropy_size;
}

size_t encode_entropy(
	uint8_t* symbols,
	size_t symbol_size,
	size_t range,
	uint8_t* output_bytes,
	uint32_t prob_bits
){
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
	size_t expected_stored_size = entropy_size + 1 + (maximum_bits_per_symbol*symbol_size + 8 - 1)/8;

	size_t expected_raw_size = (prob_bits*range + 8 - 1)/8;
	size_t expected_clamped_size = 2*(maximum_bits_per_symbol - 1)*((prob_bits - 1)/4 + 2);
	expected_clamped_size += prob_bits*2;

	uint8_t lower_clamps[((prob_bits - 1)/4 + 2)];
	uint8_t upper_clamps[((prob_bits - 1)/4 + 2)];

	int size_bits = 0;
	for(int i=0;i<range/2 - 1;i++){
		while(freqs[i] >= (1<<size_bits)){
			if(size_bits == 0){
				size_bits = 1;
				lower_clamps[0] = i;
			}
			else if(size_bits == 1){
				size_bits = 4;
				lower_clamps[1] = i;
			}
			else{
				lower_clamps[size_bits/4 + 1] = i;
				size_bits += 4;
			}
		}
		if(size_bits > prob_bits){
			size_bits = prob_bits;
		}
		expected_clamped_size += size_bits;
	}
	size_bits = 0;
	for(int i=range - 1;i>range/2;i--){
		while(freqs[i] >= (1<<size_bits)){
			if(size_bits == 0){
				size_bits = 1;
				upper_clamps[0] = 511 - i;
			}
			else if(size_bits == 1){
				size_bits = 4;
				upper_clamps[1] = 511 - i;
			}
			else{
				upper_clamps[size_bits/4 + 1] = 511 - i;
				size_bits += 4;
			}
		}
		if(size_bits > prob_bits){
			size_bits = prob_bits;
		}
		expected_clamped_size += size_bits;
	}
	expected_clamped_size = (expected_clamped_size + 8 - 1)/8;
	//printf("tab %d %d\n",(int)expected_raw_size,(int)expected_clamped_size);
	if(expected_raw_size < expected_clamped_size){
		output_bytes[entropy_size++] = (1<<7) + (prob_bits<<2) + 1;
		uint8_t bits_remaining = 8;
		uint8_t current_byte;
		for(int i=0;i<range;i++){
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[i],maximum_bits_per_symbol);
		}
		if(bits_remaining != 8){
			output_bytes[entropy_size++] = current_byte;
		}
	}
	else{
		output_bytes[entropy_size++] = (1<<7) + (prob_bits<<2) + 2;
		uint8_t bits_remaining = 8;
		uint8_t current_byte;
		for(int i=0;i<((prob_bits - 1)/4 + 2);i++){
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)lower_clamps[i],maximum_bits_per_symbol - 1);
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)upper_clamps[i],maximum_bits_per_symbol - 1);
		}
		int size_bits = 0;
		for(int i=0;i<range/2 - 1;i++){
			while(freqs[i] >= (1<<size_bits)){
				if(size_bits == 0){
					size_bits = 1;
					lower_clamps[0] = i;
				}
				else if(size_bits == 1){
					size_bits = 4;
					lower_clamps[1] = i;
				}
				else{
					lower_clamps[size_bits/4 + 1] = i;
					size_bits += 4;
				}
			}
			if(size_bits > prob_bits){
				size_bits = prob_bits;
			}
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[i],size_bits);
		}
		stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[range/2 - 1],maximum_bits_per_symbol);
		size_bits = 0;
		for(int i=range - 1;i>range/2;i--){
			while(freqs[i] >= (1<<size_bits)){
				if(size_bits == 0){
					size_bits = 1;
					upper_clamps[0] = 511 - i;
				}
				else if(size_bits == 1){
					size_bits = 4;
					upper_clamps[1] = 511 - i;
				}
				else{
					upper_clamps[size_bits/4 + 1] = 511 - i;
					size_bits += 4;
				}
			}
			if(size_bits > prob_bits){
				size_bits = prob_bits;
			}
			stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[i],size_bits);
		}
		stuffer(output_bytes,&entropy_size,&current_byte,&bits_remaining,(uint32_t)freqs[range/2],maximum_bits_per_symbol);
		if(bits_remaining != 8){
			output_bytes[entropy_size++] = current_byte;
		}
	}

//end encode table
	static size_t out_max_size = 32<<20; // 32MB
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	uint32_t* out_buf = new uint32_t[out_max_elems];
	uint32_t* out_end = out_buf + out_max_elems;

	Rans64EncSymbol esyms[range];

	for (int i=0; i < range; i++) {
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

	while(rans_begin < out_end){
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>24);
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>16);
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>8);
		output_bytes[entropy_size++] = (uint8_t)(*rans_begin);
		rans_begin++;
	}

	delete[] freqs;
	delete[] cum_freqs;
	delete[] out_buf;

	if(expected_stored_size < entropy_size){
		entropy_size = 0;
		write_varint(output_bytes, &entropy_size, range-1);
		write_varint(output_bytes, &entropy_size, symbol_size);
		output_bytes[entropy_size++] = 0;
	}
	
	//printf("entropy: %d\n",(int)entropy_size);
	return entropy_size;
}

#endif //ENTROPY_ENCODING_HEADER
