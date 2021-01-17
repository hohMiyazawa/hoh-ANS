#ifndef CHANNEL_ENCODE_HEADER
#define CHANNEL_ENCODE_HEADER

#include "rans64.hpp"

int ranscode_symbols_256(
	uint8_t* symbols,
	size_t symbol_size,
	SymbolStats_256 stats,
	uint32_t prob_bits,
	uint32_t* out_buf,
	uint32_t* out_end,
	uint32_t* rans_begin
){
	static const uint32_t prob_scale = 1 << prob_bits;

	static size_t out_max_size = 32<<20; // 32MB
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	out_buf = new uint32_t[out_max_elems];
	out_end = out_buf + out_max_elems;

	Rans64EncSymbol esyms[256];

	for (int i=0; i < 256; i++) {
		Rans64EncSymbolInit(&esyms[i], stats.cum_freqs[i], stats.freqs[i], prob_bits);
	}

	Rans64State rans;
	Rans64EncInit(&rans);

	uint32_t* ptr = out_end; // *end* of output buffer
	for (size_t i=symbol_size; i > 0; i--) { // NB: working in reverse!
		int s = symbols[i-1];
		Rans64EncPutSymbol(&rans, &ptr, &esyms[s], prob_bits);
	}
	Rans64EncFlush(&rans, &ptr);
	rans_begin = ptr;

	int bytes = (int) ((out_end - rans_begin) * sizeof(uint32_t));
	//printf("rANS: %d bytes\n", bytes);
	return bytes;
}

int ranscode_symbols_512(
	uint16_t* symbols,
	size_t symbol_size,
	SymbolStats_512 stats,
	uint32_t prob_bits,
	uint32_t* out_buf,
	uint32_t* out_end,
	uint32_t* rans_begin
){
	static const uint32_t prob_scale = 1 << prob_bits;

	static size_t out_max_size = 32<<20; // 32MB
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	out_buf = new uint32_t[out_max_elems];
	out_end = out_buf + out_max_elems;

	Rans64EncSymbol esyms[512];

	for (int i=0; i < 512; i++) {
		Rans64EncSymbolInit(&esyms[i], stats.cum_freqs[i], stats.freqs[i], prob_bits);
	}

	Rans64State rans;
	Rans64EncInit(&rans);

	uint32_t* ptr = out_end; // *end* of output buffer
	for (size_t i=symbol_size; i > 0; i--) { // NB: working in reverse!
		int s = symbols[i-1];
		Rans64EncPutSymbol(&rans, &ptr, &esyms[s], prob_bits);
	}
	Rans64EncFlush(&rans, &ptr);
	rans_begin = ptr;

	int bytes = (int) ((out_end - rans_begin) * sizeof(uint32_t));
	//printf("rANS: %d bytes\n", bytes);
	return bytes;
}

int ranscode_symbols_1024(
	uint16_t* symbols,
	size_t symbol_size,
	SymbolStats_1024 stats,
	uint32_t prob_bits,
	uint32_t* out_buf,
	uint32_t* out_end,
	uint32_t* rans_begin
){
	static const uint32_t prob_scale = 1 << prob_bits;

	static size_t out_max_size = 32<<20; // 32MB
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	out_buf = new uint32_t[out_max_elems];
	out_end = out_buf + out_max_elems;

	Rans64EncSymbol esyms[1024];

	for (int i=0; i < 1024; i++) {
		Rans64EncSymbolInit(&esyms[i], stats.cum_freqs[i], stats.freqs[i], prob_bits);
	}

	Rans64State rans;
	Rans64EncInit(&rans);

	uint32_t* ptr = out_end; // *end* of output buffer
	for (size_t i=symbol_size; i > 0; i--) { // NB: working in reverse!
		int s = symbols[i-1];
		Rans64EncPutSymbol(&rans, &ptr, &esyms[s], prob_bits);
	}
	Rans64EncFlush(&rans, &ptr);
	rans_begin = ptr;

	int bytes = (int) ((out_end - rans_begin) * sizeof(uint32_t));
	//printf("rANS: %d bytes\n", bytes);
	return bytes;
}

int table_code(uint32_t* data, uint32_t prob_bits, uint32_t range_bits){
	uint32_t range = 1<<range_bits;
	int bits = 0;
	bits += ((prob_bits + (prob_bits % 4))/4 + 1)*2*(range_bits - 1);
	int i = 0;
	int size_bits = 0;
	for(int i=0;i<(range/2);i++){
		if(size_bits == 0 && data[i] != 0){
			size_bits = 1;
			bits++;
		}
		else if(size_bits == 1 && data[1] > 1){
			size_bits = 4;
			bits += 4;
		}
		else{
			while(data[i] > (1<<size_bits) - 1){
				size_bits += 4;
			}
			bits += size_bits;
		}
	}
	size_bits = 0;
	for(int i=range-1;i >= range/2;i--){
		if(size_bits == 0 && data[i] != 0){
			size_bits = 1;
			bits++;
		}
		else if(size_bits == 1 && data[1] > 1){
			size_bits = 4;
			bits += 4;
		}
		else{
			while(data[i] > (1<<size_bits) - 1){
				size_bits += 4;
			}
			bits += size_bits;
		}
	}
	return (bits + (bits % 8))/8;
}

int channel_encode(
	uint8_t* symbols,
	size_t symbol_size,
	uint32_t prob_bits,
	uint32_t* out_buf,
	uint32_t* out_end,
	uint32_t* rans_begin
){
	uint32_t prob_scale = 1 << prob_bits;
	SymbolStats_256 stats;
	stats.count_freqs(symbols, symbol_size);
	stats.normalize_freqs(prob_scale);

	uint32_t* table_data = new uint32_t[256];
	for(int i=0;i<256;i++){
		table_data[i] = stats.freqs[i];
	}
	int size = table_code(table_data, prob_bits, 8);

	delete[] table_data;

	return 1 + size + ranscode_symbols_256(
		symbols,
		symbol_size,
		stats,
		prob_bits,
		out_buf,
		out_end,
		rans_begin
	);
}

int channel_encode(
	uint16_t* symbols,
	size_t symbol_size,
	int depth,
	uint32_t prob_bits,
	uint32_t* out_buf,
	uint32_t* out_end,
	uint32_t* rans_begin
){
	uint32_t prob_scale = 1 << prob_bits;
	if(depth == 9){
		SymbolStats_512 stats;
		stats.count_freqs(symbols, symbol_size);
		stats.normalize_freqs(prob_scale);

		/*float deviation = stats.deviation();
		printf("std_dev: %.6f\n",deviation);
		SymbolStats_512 supplemental_stats;
		supplemental_stats.laplace(deviation,prob_scale);
		supplemental_stats.normalize_freqs(prob_scale);*/

		uint32_t* table_data = new uint32_t[512];
		for(int i=0;i<512;i++){
			table_data[i] = stats.freqs[i];
			//printf("%d %d\n",i,(int)(stats.freqs[i]) - (int)(supplemental_stats.freqs[i]));
			//printf("%d %d\n",i,(int)(stats.freqs[i]));
		}
		//printf("table size: %d\n",table_code(table_data, prob_bits, 9));
		int size = table_code(table_data, prob_bits, 9);

		delete[] table_data;

		return 1 + size + ranscode_symbols_512(
			symbols,
			symbol_size,
			stats,
			prob_bits,
			out_buf,
			out_end,
			rans_begin
		);
	}
	else if(depth == 10){
		SymbolStats_1024 stats;
		stats.count_freqs(symbols, symbol_size);
		stats.normalize_freqs(prob_scale);

		uint32_t* table_data = new uint32_t[1024];
		for(int i=0;i<1024;i++){
			table_data[i] = stats.freqs[i];
			//printf("%d %d\n",i,(int)(stats.freqs[i]) - (int)(supplemental_stats.freqs[i]));
			//printf("%d %d\n",i,(int)(stats.freqs[i]));
		}
		//printf("table size: %d\n",table_code(table_data, prob_bits, 10));
		int size = table_code(table_data, prob_bits, 10);

		delete[] table_data;

		return 1 + size + ranscode_symbols_1024(
			symbols,
			symbol_size,
			stats,
			prob_bits,
			out_buf,
			out_end,
			rans_begin
		);
	}
}

#endif // CHANNEL_ENCODE_HEADER
