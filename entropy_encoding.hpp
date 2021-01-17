#ifndef ENTROPY_ENCODING_HEADER
#define ENTROPY_ENCODING_HEADER

#include <assert.h>

#include "rans64.hpp"
#include "varint.hpp"
#include "symbolstats.hpp"

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
	uint8_t effort
){
	uint8_t maximum_bits_per_symbol = 0;
	for(size_t check = (range - 1);check;check = (check>>1)){
		maximum_bits_per_symbol++;
	}
	size_t maximum_size = 10 + range*2 + (symbol_size*maximum_bits_per_symbol + 8 - 1)/8;
	size_t entropy_size = 0;
	output_bytes = new uint8_t[maximum_size];

	printf("maximum_bits_per_symbol: %d\n",(int)maximum_bits_per_symbol);

	uint32_t prob_bits = 14;
	uint32_t prob_scale = 1 << prob_bits;
	uint32_t* freqs = new uint32_t[range];
	uint32_t* cum_freqs = new uint32_t[range + 1];
	for(size_t i=0;i<symbol_size;i++){
		freqs[symbols[i]]++;
	}
	printf("prob_scale: %d\n",(int)prob_scale);
	normalize_freqs(freqs,cum_freqs,range,prob_scale);
//encode table here
	write_varint(output_bytes, &entropy_size, range-1);
	write_varint(output_bytes, &entropy_size, symbol_size-1);
	output_bytes[entropy_size++] = (1<<7) + (prob_bits<<2) + 1;
	uint8_t bits_remaining = 8;
	uint8_t current_byte;
	for(int i=0;i<range;i++){
		current_byte += freqs[i] >>(prob_bits - bits_remaining);
		output_bytes[entropy_size++] = current_byte;
		if(bits_remaining + 8 >= prob_bits){
			bits_remaining = (8 - (prob_bits - bits_remaining));
		}
		else{
			output_bytes[entropy_size++] = (uint8_t)((freqs[i] >>(prob_bits - 8 - bits_remaining)) % 256);
			bits_remaining = 16 + bits_remaining - prob_bits;
		}
		current_byte = (uint8_t)((freqs[i]<<bits_remaining) % 256);
	}
	if(bits_remaining != 8){
		output_bytes[entropy_size++] = current_byte;
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

	int bytes = (int) ((out_end - rans_begin) * sizeof(uint32_t));
	while(rans_begin < out_end){
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>24);
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>16);
		output_bytes[entropy_size++] = (uint8_t)((*rans_begin)>>8);
		output_bytes[entropy_size++] = (uint8_t)(*rans_begin);
		rans_begin++;
	}
	
	printf("rANS: %d bytes\n", bytes);

	delete[] freqs;
	delete[] cum_freqs;
	delete[] out_buf;
	
	printf("woof: %d\n",(int)entropy_size);
	return entropy_size;
}

#endif //ENTROPY_ENCODING_HEADER
