#ifndef BITIMAGE_HEADER
#define BITIMAGE_HEADER

#include "rans64.h"

struct SymbolStats_2{
	uint32_t freqs[2];
	uint32_t cum_freqs[3];

	void count_freqs(uint8_t const* in, size_t nbytes);
	void calc_cum_freqs();
	void normalize_freqs(uint32_t target_total);
};

void SymbolStats_2::count_freqs(uint8_t const* in, size_t nbytes){
	for (int i=0; i < 2; i++)
		freqs[i] = 0;

	for (size_t i=0; i < nbytes; i++)
		freqs[in[i]]++;
}

void SymbolStats_2::calc_cum_freqs(){
	cum_freqs[0] = 0;
	for (int i=0; i < 2; i++)
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
}

void SymbolStats_2::normalize_freqs(uint32_t target_total){
	int sum = freqs[0] + freqs[1];
	int num1 = target_total*freqs[0]/sum;
	int num2 = target_total - num1;
	if(num1 == 0 && freqs[0] != 0){
		num1++;
		num2--;
	}
	if(num2 == 0 && freqs[1] != 0){
		num1--;
		num2++;
	}
	freqs[0] = num1;
	freqs[1] = num2;
	calc_cum_freqs();
}

int ranscode_symbols_2(
	uint8_t* symbols,
	size_t symbol_size,
	SymbolStats_2 stats,
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

	Rans64EncSymbol esyms[2];

	for (int i=0; i < 2; i++) {
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

int bitcontext_encode(
	uint8_t* symbols,
	size_t symbol_size,
	uint32_t prob_bits,
	uint32_t* out_buf,
	uint32_t* out_end,
	uint32_t* rans_begin
){
	uint32_t prob_scale = 1 << prob_bits;
	SymbolStats_2 stats;
	stats.count_freqs(symbols, symbol_size);
	stats.normalize_freqs(prob_scale);

	uint32_t* table_data = new uint32_t[2];
	for(int i=0;i<2;i++){
		table_data[i] = stats.freqs[i];
	}
	int size = (prob_bits + (prob_bits % 8))/8;//int size = table_code(table_data, prob_bits, 8);

	delete[] table_data;

	return size + ranscode_symbols_2(
		symbols,
		symbol_size,
		stats,
		prob_bits,
		out_buf,
		out_end,
		rans_begin
	);
}

int bitimage_encode(
	uint8_t* data,
	size_t size,
	int width,
	int height
){
	uint8_t* context_00 = new uint8_t[size];
	uint8_t* context_01 = new uint8_t[size];
	uint8_t* context_10 = new uint8_t[size];
	uint8_t* context_11 = new uint8_t[size];
	uint8_t* context_100 = new uint8_t[size];
	uint8_t* context_101 = new uint8_t[size];
	uint8_t* context_110 = new uint8_t[size];
	uint8_t* context_111 = new uint8_t[size];
	size_t context_00_size = 0;
	size_t context_01_size = 0;
	size_t context_10_size = 0;
	size_t context_11_size = 0;
	size_t context_100_size = 0;
	size_t context_101_size = 0;
	size_t context_110_size = 0;
	size_t context_111_size = 0;

	uint32_t* dummy1;
	uint32_t* dummy2;
	uint32_t* dummy3;
	uint32_t prob_bits = 12;

	uint8_t forige = 0;
	uint8_t forige_TL = 0;
	uint8_t top_row[width];
	for(int i=0;i<width;i++){
		top_row[i] = 0;
	}
	for (int i=0; i < size; i++){
		uint8_t L = forige;
		uint8_t T = top_row[i % width];
		uint8_t TL = forige_TL;

		if(TL == 1){
			if(L == 0 && T == 0){
				context_100[context_100_size++] = data[i];
			}
			else if(L == 0 && T == 1){
				context_101[context_101_size++] = data[i];
			}
			else if(L == 1 && T == 0){
				context_110[context_110_size++] = data[i];
			}
			else{
				context_111[context_111_size++] = data[i];
			}
		}
		else{
			if(L == 0 && T == 0){
				context_00[context_00_size++] = data[i];
			}
			else if(L == 0 && T == 1){
				context_01[context_01_size++] = data[i];
			}
			else if(L == 1 && T == 0){
				context_10[context_10_size++] = data[i];
			}
			else{
				context_11[context_11_size++] = data[i];
			}
		}
		forige = data[i];
		forige_TL = top_row[i % width];
		top_row[i % width] = data[i];
	}

	int size1 = bitcontext_encode(
		context_00,
		context_00_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	int size2 = bitcontext_encode(
		context_01,
		context_01_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	int size3 = bitcontext_encode(
		context_10,
		context_10_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	int size4 = bitcontext_encode(
		context_11,
		context_11_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);

	int size5 = bitcontext_encode(
		context_100,
		context_100_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	int size6 = bitcontext_encode(
		context_101,
		context_101_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	int size7 = bitcontext_encode(
		context_110,
		context_110_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	int size8 = bitcontext_encode(
		context_111,
		context_111_size,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	/*printf("lengths %d %d %d %d %d %d %d %d = %d\n",
		(int)context_00_size,(int)context_01_size,(int)context_10_size,(int)context_11_size,
		(int)context_100_size,(int)context_101_size,(int)context_110_size,(int)context_111_size,
		(int)size
	);
	printf("sizes %d %d %d %d %d %d %d %d\n",size1,size2,size3,size4,size5,size6,size7,size8);
	printf("total %d\n",1 + size1 + size2 + size3 + size4 + size5 + size6 + size7 + size8);*/
	return 1 + 2 + size1 + size2 + size3 + size4 + size5 + size6 + size7 + size8;
}

#endif // BITIMAGE_HEADER
