#include "platform.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <cstdlib>
#include <cmath>
#include <math.h>

#include "rans64.h"
#include "file_io.h"
#include "symbolstats.h"
#include "channel.h"

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
	printf("rANS: %d bytes\n", bytes);
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
	printf("rANS: %d bytes\n", bytes);
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
	printf("rANS: %d bytes\n", bytes);
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
	uint16_t* symbols,
	size_t symbol_size,
	int depth,
	uint32_t prob_bits,
	uint32_t* out_buf,
	uint32_t* out_end,
	uint32_t* rans_begin
){
	static const uint32_t prob_scale = 1 << prob_bits;
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

uint8_t midpoint(uint8_t a, uint8_t b){
	return a + (b - a) / 2;
}

uint16_t midpoint(uint16_t a, uint16_t b){
	return a + (b - a) / 2;
}

uint8_t average3(uint8_t a, uint8_t b, uint8_t c){
	return (uint8_t)(((int)a + (int)b + (int)c)/3);
}

uint16_t average3(uint16_t a, uint16_t b, uint16_t c){
	return (uint16_t)(((int)a + (int)b + (int)c)/3);
}

uint8_t paeth(uint8_t A,uint8_t B,uint8_t C){
	int p = A + B - C;
	int Ap = std::abs(A - p);
	int Bp = std::abs(B - p);
	int Cp = std::abs(C - p);
	if(Ap < Bp){
		if(Ap < Cp){
			return (uint8_t)Ap;
		}
		return (uint8_t)Cp;
	}
	else{
		if(Bp < Cp){
			return (uint8_t)Bp;
		}
		return (uint8_t)Cp;
	}
}

uint16_t paeth(uint16_t A,uint16_t B,uint16_t C){
	int p = A + B - C;
	int Ap = std::abs(A - p);
	int Bp = std::abs(B - p);
	int Cp = std::abs(C - p);
	if(Ap < Bp){
		if(Ap < Cp){
			return (uint16_t)Ap;
		}
		return (uint16_t)Cp;
	}
	else{
		if(Bp < Cp){
			return (uint16_t)Bp;
		}
		return (uint16_t)Cp;
	}
}

uint16_t* channelpredict(uint8_t* data, size_t size, int width, int height, uint32_t predictors){
	uint8_t forige = 128;
	int best_pred[width];
	for(int i=0;i<width;i++){
		best_pred[i] = 0;
	}
	uint8_t top_row[width];
	for(int i=0;i<width;i++){
		top_row[i] = 128;
	}
	uint16_t* out_buf = new uint16_t[size];
	for (int i=0; i < size; i++){
		uint8_t L = forige;
		uint8_t T = top_row[i % width];
		uint8_t TL = top_row[(i + width - 1) % width];
		uint8_t TR = top_row[(i + width + 1) % width];
		uint8_t predictions[15] = {
			L,
			T,
			TL,
			TR,
			midpoint(L,T),
			midpoint(L,TL),
			midpoint(TL,T),
			midpoint(T,TR),
			paeth(L,TL,T),
			average3(L,L,TL),
			average3(L,TL,TL),
			average3(TL,TL,T),
			average3(TL,T,T),
			average3(T,T,TR),
			average3(T,TR,TR)
/*,
			(L*2 + TL)/3,
			(L + TL*2)/3,
			(TL*2 + T)/3,
			(TL + T*2)/3,
			(T*2 + TR)/3,
			(T + TR*2)/3,
			(L + TL + T + TR)/4,
			((L + TR)/2 + T)/2*/
		};
		//out_buf[i] = data[i] - predictions[best_pred[i % width]] + 256;
		out_buf[i] = data[i] - midpoint(predictions[best_pred[i % width]],predictions[best_pred[(i + width - 1) % width]]) + 256;
		forige = data[i];
		top_row[i % width] = data[i];
		int best_val = std::abs(data[i] - predictions[0]);
		best_pred[i % width] = 0;
		for(int j=1;j<15;j++){
			int val = std::abs(data[i] - predictions[j]);
			if(val < best_val && (predictors & (1 << j))){
				best_val = val;
				best_pred[i % width] = j;
			}
		}
	}
	/*for(size_t i=0;i<size;i++){
		out_buf[i] = data[i] - forige + 256;
		forige = data[i];
	}*/
	return out_buf;
}

uint16_t* channelpredict(uint16_t* data, size_t size, int width, int height, int depth, uint32_t predictors){
	int centre = 1<<(depth - 1);

	uint16_t forige = 256;
	int best_pred[width];
	for(int i=0;i<width;i++){
		best_pred[i] = 0;
	}
	uint16_t top_row[width];
	for(int i=0;i<width;i++){
		top_row[i] = 256;
	}
	uint16_t* out_buf = new uint16_t[size];
	for (int i=0; i < size; i++){
		uint16_t L = forige;
		uint16_t T = top_row[i % width];
		uint16_t TL = top_row[(i + width - 1) % width];
		uint16_t TR = top_row[(i + width + 1) % width];
		uint16_t predictions[15] = {
			L,
			T,
			TL,
			TR,
			midpoint(L,T),
			midpoint(L,TL),
			midpoint(TL,T),
			midpoint(T,TR),
			paeth(L,TL,T),
			average3(L,L,TL),
			average3(L,TL,TL),
			average3(TL,TL,T),
			average3(TL,T,T),
			average3(T,T,TR),
			average3(T,TR,TR)
/*,
			(L*2 + TL)/3,
			(L + TL*2)/3,
			(TL*2 + T)/3,
			(TL + T*2)/3,
			(T*2 + TR)/3,
			(T + TR*2)/3,
			(L + TL + T + TR)/4,
			((L + TR)/2 + T)/2*/
		};
		//out_buf[i] = data[i] - predictions[best_pred[i % width]] + 256;
		out_buf[i] = data[i] - midpoint(predictions[best_pred[i % width]],predictions[best_pred[(i + width - 1) % width]]) + centre;
		forige = data[i];
		top_row[i % width] = data[i];
		int best_val = std::abs(data[i] - predictions[0]);
		best_pred[i % width] = 0;
		for(int j=1;j<15;j++){
			int val = std::abs(data[i] - predictions[j]);
			if(val < best_val && (predictors & (1 << j))){
				best_val = val;
				best_pred[i % width] = j;
			}
		}
	}
	return out_buf;
}

void print_usage(){
	printf("usage: hoh infile.raw width height\n\n");
	printf("The input file must consist of raw 8bit RGB bytes\n");
	printf("You can make such a file with imagemagick:\n");
	printf("convert input.png -depth 8 rgb:infile.raw\n\n");
	printf("Output will at present not be written because file IO is tedious\n");
}

int main(int argc, char *argv[]){
	if(argc < 4){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	int width = atoi(argv[2]);
	int height = atoi(argv[3]);
	if(width == 0 || height == 0){
		printf("invalid width or height\n");
		print_usage();
		return 2;
	}
	int cruncher_mode = 0;
	if(argc > 4 && strcpy(argv[4],"crunch")){
		cruncher_mode = 1;
		printf("cruncher mode activated\n");
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);

	static const uint32_t prob_bits = 16;
	static const uint32_t prob_scale = 1 << prob_bits;

	/*static size_t out_max_size = 32<<20; // 32MB
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	uint32_t* out_buf = new uint32_t[out_max_elems];
	uint32_t* out_end = out_buf + out_max_elems;
	uint8_t* dec_bytes = new uint8_t[in_size];*/

	// try rANS encode

	/*size_t channel_0_size;
	uint8_t* channel_0 = channel_picker(in_bytes, in_size, 3, 0, &channel_0_size);

	SymbolStats_256 stats;
	stats.count_freqs(channel_0, channel_0_size);
	stats.normalize_freqs(prob_scale);

	uint32_t *out_buf_0;
	uint32_t *out_end_0;
	uint32_t *rans_begin_0;

	ranscode_symbols_256(
		channel_0,
		channel_0_size,
		stats,
		prob_bits,
		out_buf_0,
		out_end_0,
		rans_begin_0
	);


	size_t channel_1_size;
	uint8_t* channel_1 = channel_picker(in_bytes, in_size, 3, 1, &channel_1_size);

	stats.count_freqs(channel_1, channel_1_size);
	stats.normalize_freqs(prob_scale);

	uint32_t *out_buf_1;
	uint32_t *out_end_1;
	uint32_t *rans_begin_1;

	ranscode_symbols_256(
		channel_1,
		channel_1_size,
		stats,
		prob_bits,
		out_buf_1,
		out_end_1,
		rans_begin_1
	);


	size_t channel_2_size;
	uint8_t* channel_2 = channel_picker(in_bytes, in_size, 3, 2, &channel_2_size);

	stats.count_freqs(channel_2, channel_2_size);
	stats.normalize_freqs(prob_scale);

	uint32_t *out_buf_2;
	uint32_t *out_end_2;
	uint32_t *rans_begin_2;

	ranscode_symbols_256(
		channel_2,
		channel_2_size,
		stats,
		prob_bits,
		out_buf_2,
		out_end_2,
		rans_begin_2
	);*/

	int rgb_size = 0;
	int subtract_green_size = 0;
	int yiq_size = 0;

	/*printf("predict\n");

	uint16_t* predict_0 = channelpredict(channel_0, channel_0_size, width, height);
	uint32_t* pred_buf_0;
	uint32_t* pred_end_0;
	uint32_t* pred_rans_begin_0;
	rgb_size += channel_encode(
		predict_0,
		channel_0_size,
		9,
		prob_bits,
		pred_buf_0,
		pred_end_0,
		pred_rans_begin_0
	);*/

	/*uint16_t* predict_1 = channelpredict(channel_1, channel_1_size, width, height);
	uint32_t* pred_buf_1;
	uint32_t* pred_end_1;
	uint32_t* pred_rans_begin_1;
	int green_channel_size = channel_encode(
		predict_1,
		channel_1_size,
		9,
		prob_bits,
		pred_buf_1,
		pred_end_1,
		pred_rans_begin_1
	);
	rgb_size += green_channel_size;
	subtract_green_size += green_channel_size;*/

	/*uint16_t* predict_2 = channelpredict(channel_2, channel_2_size, width, height);
	uint32_t* pred_buf_2;
	uint32_t* pred_end_2;
	uint32_t* pred_rans_begin_2;
	rgb_size += channel_encode(
		predict_2,
		channel_2_size,
		9,
		prob_bits,
		pred_buf_2,
		pred_end_2,
		pred_rans_begin_2
	);*/

	//printf("subtract green\n");

	size_t split_size = in_size/3;
	uint8_t* GREEN = new uint8_t[split_size];
	uint16_t* RED_G = new uint16_t[split_size];
	uint16_t* BLUE_G = new uint16_t[split_size];
	subtract_green(in_bytes, in_size, GREEN, RED_G, BLUE_G);

	palette_compact(GREEN,split_size);//costs 32 bytes, but who's counting?

	if(cruncher_mode){
		uint16_t* predict_1 = channelpredict(GREEN, split_size, width, height, 0xFFFF);
		uint32_t* pred_buf_1;
		uint32_t* pred_end_1;
		uint32_t* pred_rans_begin_1;
		int green_channel_size = channel_encode(
			predict_1,
			split_size,
			9,
			prob_bits,
			pred_buf_1,
			pred_end_1,
			pred_rans_begin_1
		);
		uint32_t green_mask = 0xFFFF;
		int best_found = -1;
		for(int i=1;i<16;i++){
			predict_1 = channelpredict(GREEN, split_size, width, height, 0xFFFF ^ (1 << i));
			int temp_size = channel_encode(
				predict_1,
				split_size,
				9,
				prob_bits,
				pred_buf_1,
				pred_end_1,
				pred_rans_begin_1
			);
			if(temp_size < green_channel_size){
				green_channel_size = temp_size;
				green_mask = 0xFFFF ^ (1 << i);
				best_found = i;
			}
		}
		if(best_found != -1){
			for(int i=1;i<16;i++){
				if(best_found == i){
					continue;
				}
				predict_1 = channelpredict(GREEN, split_size, width, height, green_mask ^ (1 << i));
				int temp_size = channel_encode(
					predict_1,
					split_size,
					9,
					prob_bits,
					pred_buf_1,
					pred_end_1,
					pred_rans_begin_1
				);
				if(temp_size < green_channel_size){
					green_channel_size = temp_size;
					green_mask = green_mask ^ (1 << i);
					best_found = i;
				}
			}
		}
		for(uint32_t i=prob_bits;i < 20;i++){
			predict_1 = channelpredict(GREEN, split_size, width, height, green_mask);
			int temp_size = channel_encode(
				predict_1,
				split_size,
				9,
				i,
				pred_buf_1,
				pred_end_1,
				pred_rans_begin_1
			);
			if(temp_size < green_channel_size){
				green_channel_size = temp_size;
			}
			else{
				break;
			}
		}
		subtract_green_size += green_channel_size;

		uint16_t* predict_1_sub = channelpredict(RED_G, split_size, width, height, 10, 0xFFFF);
		uint32_t* pred_buf_1_sub;
		uint32_t* pred_end_1_sub;
		uint32_t* pred_rans_begin_1_sub;
		int sub_red_channel_size = channel_encode(
			predict_1_sub,
			split_size,
			10,
			prob_bits,
			pred_buf_1_sub,
			pred_end_1_sub,
			pred_rans_begin_1_sub
		);
		uint32_t sub_red_mask = 0xFFFF;
		for(int i=1;i<16;i++){
			predict_1_sub = channelpredict(RED_G, split_size, width, height, 10, 0xFFFF ^ (1 << i));
			int temp_size = channel_encode(
				predict_1_sub,
				split_size,
				10,
				prob_bits,
				pred_buf_1_sub,
				pred_end_1_sub,
				pred_rans_begin_1_sub
			);
			if(temp_size < sub_red_channel_size){
				sub_red_channel_size = temp_size;
				sub_red_mask = 0xFFFF ^ (1 << i);
			}
		}
		for(uint32_t i=prob_bits;i < 20;i++){
			predict_1_sub = channelpredict(RED_G, split_size, width, height, 10, sub_red_mask);
			int temp_size = channel_encode(
				predict_1_sub,
				split_size,
				10,
				i,
				pred_buf_1_sub,
				pred_end_1_sub,
				pred_rans_begin_1_sub
			);
			if(temp_size < sub_red_channel_size){
				sub_red_channel_size = temp_size;
			}
			else{
				break;
			}
		}
		subtract_green_size += sub_red_channel_size;

		uint16_t* predict_2_sub = channelpredict(BLUE_G, split_size, width, height, 10, 0xFFFF);
		uint32_t* pred_buf_2_sub;
		uint32_t* pred_end_2_sub;
		uint32_t* pred_rans_begin_2_sub;
		int sub_blue_channel_size = channel_encode(
			predict_2_sub,
			split_size,
			10,
			prob_bits,
			pred_buf_2_sub,
			pred_end_2_sub,
			pred_rans_begin_2_sub
		);
		uint32_t sub_blue_mask = 0xFFFF;
		for(int i=1;i<16;i++){
			predict_2_sub = channelpredict(BLUE_G, split_size, width, height, 10, 0xFFFF ^ (1 << i));
			int temp_size = channel_encode(
				predict_2_sub,
				split_size,
				10,
				prob_bits,
				pred_buf_2_sub,
				pred_end_2_sub,
				pred_rans_begin_2_sub
			);
			if(temp_size < sub_blue_channel_size){
				sub_blue_channel_size = temp_size;
				sub_blue_mask = 0xFFFF ^ (1 << i);
			}
		}
		for(uint32_t i=prob_bits;i < 20;i++){
			predict_1_sub = channelpredict(BLUE_G, split_size, width, height, 10, sub_blue_mask);
			int temp_size = channel_encode(
				predict_1_sub,
				split_size,
				10,
				i,
				pred_buf_1_sub,
				pred_end_1_sub,
				pred_rans_begin_1_sub
			);
			if(temp_size < sub_blue_channel_size){
				sub_blue_channel_size = temp_size;
			}
			else{
				break;
			}
		}
		subtract_green_size += sub_blue_channel_size;
	}
	else{
		uint16_t* predict_1 = channelpredict(GREEN, split_size, width, height, 0xFFFF);
		uint32_t* pred_buf_1;
		uint32_t* pred_end_1;
		uint32_t* pred_rans_begin_1;
		int green_channel_size = channel_encode(
			predict_1,
			split_size,
			9,
			prob_bits,
			pred_buf_1,
			pred_end_1,
			pred_rans_begin_1
		);
		rgb_size += green_channel_size;
		subtract_green_size += green_channel_size;

		uint16_t* predict_1_sub = channelpredict(RED_G, split_size, width, height, 10, 0xFFFF);
		uint32_t* pred_buf_1_sub;
		uint32_t* pred_end_1_sub;
		uint32_t* pred_rans_begin_1_sub;
		subtract_green_size += channel_encode(
			predict_1_sub,
			split_size,
			10,
			prob_bits,
			pred_buf_1_sub,
			pred_end_1_sub,
			pred_rans_begin_1_sub
		);

		uint16_t* predict_2_sub = channelpredict(BLUE_G, split_size, width, height, 10, 0xFFFF);
		uint32_t* pred_buf_2_sub;
		uint32_t* pred_end_2_sub;
		uint32_t* pred_rans_begin_2_sub;
		subtract_green_size += channel_encode(
			predict_2_sub,
			split_size,
			10,
			prob_bits,
			pred_buf_2_sub,
			pred_end_2_sub,
			pred_rans_begin_2_sub
		);
	}

	/*printf("YIQ transform\n");
	uint8_t*  Y = new uint8_t[split_size];
	uint16_t* I = new uint16_t[split_size];
	uint16_t* Q = new uint16_t[split_size];
	rgb_to_yiq(in_bytes, in_size, Y, I, Q);

	uint16_t* predict_Y = channelpredict(Y, split_size, width, height);
	uint32_t* pred_buf_Y;
	uint32_t* pred_end_Y;
	uint32_t* pred_rans_begin_Y;
	yiq_size += channel_encode(
		predict_Y,
		split_size,
		9,
		prob_bits,
		pred_buf_Y,
		pred_end_Y,
		pred_rans_begin_Y
	);

	uint16_t* predict_I_sub = channelpredict(I, split_size, width, height,10);
	uint32_t* pred_buf_I_sub;
	uint32_t* pred_end_I_sub;
	uint32_t* pred_rans_begin_I_sub;
	yiq_size += channel_encode(
		predict_I_sub,
		split_size,
		10,
		prob_bits,
		pred_buf_I_sub,
		pred_end_I_sub,
		pred_rans_begin_I_sub
	);

	uint16_t* predict_Q_sub = channelpredict(Q, split_size, width, height,10);
	uint32_t* pred_buf_Q_sub;
	uint32_t* pred_end_Q_sub;
	uint32_t* pred_rans_begin_Q_sub;
	yiq_size += channel_encode(
		predict_Q_sub,
		split_size,
		10,
		prob_bits,
		pred_buf_Q_sub,
		pred_end_Q_sub,
		pred_rans_begin_Q_sub
	);*/
	//printf("RGB: %d\n",rgb_size);
	//printf("S_G: %d\n",subtract_green_size);
	//printf("YIQ: %d\n",yiq_size);

	printf("Total file size: %d bytes\n",subtract_green_size + 8 + 8 + 2 + 3);
	//data size + channel offsets + width/height + transforms + identifier

	/*delete[] channel_0;
	delete[] out_buf_0;
	delete[] channel_1;
	delete[] out_buf_1;
	delete[] channel_2;
	delete[] out_buf_2;*/
	delete[] GREEN;
	delete[] RED_G;
	delete[] BLUE_G;
	//delete[] Y;
	//delete[] I;
	//delete[] Q;

	/*Rans64EncSymbol esyms[256];
	Rans64DecSymbol dsyms[256];

	for (int i=0; i < 256; i++) {
		Rans64EncSymbolInit(&esyms[i], stats.cum_freqs[i], stats.freqs[i], prob_bits);
		Rans64DecSymbolInit(&dsyms[i], stats.cum_freqs[i], stats.freqs[i]);
	}*/

	/*
	// cumlative->symbol table
	// this is super brute force
	uint8_t cum2sym[prob_scale];
	for (int s=0; s < 256; s++)
		for (uint32_t i=stats.cum_freqs[s]; i < stats.cum_freqs[s+1]; i++)
			cum2sym[i] = s;
	// try rANS decode
	{

		Rans64State rans;
		uint32_t* ptr = rans_begin;
		Rans64DecInit(&rans, &ptr);

		for (size_t i=0; i < in_size; i++) {
			uint32_t s = cum2sym[Rans64DecGet(&rans, prob_bits)];
			dec_bytes[i] = (uint8_t) s;
			Rans64DecAdvanceSymbol(&rans, &ptr, &dsyms[s], prob_bits);
		}
	}*/

	//delete[] dec_bytes;
	delete[] in_bytes;
	return 0;
}
