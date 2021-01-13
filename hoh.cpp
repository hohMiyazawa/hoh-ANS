#include "platform.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <cstdlib>
#include <cmath>
#include <math.h>

#include "rans64.hpp"
#include "file_io.hpp"
#include "symbolstats.hpp"
#include "channel.hpp"
#include "patches.hpp"
#include "lz.hpp"
#include "channel_encode.hpp"
#include "bitimage.hpp"

uint8_t midpoint(uint8_t a, uint8_t b){
	return a + (b - a) / 2;
}

uint8_t median(uint8_t a, uint8_t b, uint8_t c){
	if(a > b){
		if(b > c){
			return b;
		}
		else if(c > a){
			return a;
		}
		else{
			return c;
		}
	}
	else{
		if(b < c){
			return b;
		}
		else if(c > a){
			return c;
		}
		else{
			return a;
		}
	}
}

uint16_t median(uint16_t a, uint16_t b, uint16_t c){
	if(a > b){
		if(b > c){
			return b;
		}
		else if(c > a){
			return a;
		}
		else{
			return c;
		}
	}
	else{
		if(b < c){
			return b;
		}
		else if(c > a){
			return c;
		}
		else{
			return a;
		}
	}
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

uint16_t* channelpredict_fastpath(uint8_t* data, size_t size, int width, int height){
	uint8_t forige = 128;
	uint8_t forige_TL = 128;
	uint8_t top_row[width];
	for(int i=0;i<width;i++){
		top_row[i] = 128;
	}
	uint16_t* out_buf = new uint16_t[size];
	for (int i=0; i < size; i++){
		uint8_t L = forige;
		uint8_t T = top_row[i % width];
		uint8_t TL = forige_TL;
		uint8_t predictions = median(T,L,T+L-TL);

		out_buf[i] = data[i] - predictions + 256;
		forige = data[i];
		forige_TL = top_row[i % width];
		top_row[i % width] = data[i];
	}
	return out_buf;
}

uint16_t* channelpredict(uint8_t* data, size_t size, int width, int height, uint32_t predictors){
	if(predictors == 0b0000000000010000){
		return channelpredict_fastpath(data, size, width, height);
	}
	uint8_t forige = 128;
	uint8_t forige_TL = 128;
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
		uint8_t TL = forige_TL;
		uint8_t TR = top_row[(i + width + 1) % width];
		uint8_t predictions[16] = {
			L,
			T,
			TL,
			TR,
			median(T,L,T+L-TL),
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
		forige_TL = top_row[i % width];
		top_row[i % width] = data[i];
		int best_val = 512;
		best_pred[i % width] = 0;
		for(int j=0;j<16;j++){
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

uint16_t* channelpredict_fastpath(uint16_t* data, size_t size, int width, int height, int depth){
	int centre = 1<<depth;

	uint16_t forige = centre/2;
	uint16_t forige_TL = centre/2;
	uint16_t top_row[width];
	for(int i=0;i<width;i++){
		top_row[i] = centre/2;
	}
	uint16_t* out_buf = new uint16_t[size];
	for (int i=0; i < size; i++){
		uint16_t L = forige;
		uint16_t T = top_row[i % width];
		uint16_t TL = forige_TL;
		uint16_t predictions = median(T,L,T+L-TL);
		out_buf[i] = data[i] - predictions + centre;
		forige = data[i];
		forige_TL = top_row[i % width];
		top_row[i % width] = data[i];
	}
	return out_buf;
}

uint16_t* channelpredict(uint16_t* data, size_t size, int width, int height, int depth, uint32_t predictors){
	if(predictors == 0b0000000000010000){
		return channelpredict_fastpath(data, size, width, height, depth);
	}
	int centre = 1<<depth;

	uint16_t forige = centre/2;
	uint16_t forige_TL = centre/2;
	int best_pred[width];
	for(int i=0;i<width;i++){
		best_pred[i] = 0;
	}
	uint16_t top_row[width];
	for(int i=0;i<width;i++){
		top_row[i] = centre/2;
	}
	uint16_t* out_buf = new uint16_t[size];
	for (int i=0; i < size; i++){
		uint16_t L = forige;
		uint16_t T = top_row[i % width];
		uint16_t TL = forige_TL;
		uint16_t TR = top_row[(i + width + 1) % width];
		uint16_t predictions[16] = {
			L,
			T,
			TL,
			TR,
			median(T,L,T+L-TL),
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
		//out_buf[i] = data[i] - predictions[best_pred[i % width]] + centre;
		out_buf[i] = data[i] - midpoint(predictions[best_pred[i % width]],predictions[best_pred[(i + width - 1) % width]]) + centre;
		forige = data[i];
		forige_TL = top_row[i % width];
		top_row[i % width] = data[i];
		int best_val = centre*2;
		best_pred[i % width] = 0;
		for(int j=0;j<16;j++){
			int val = std::abs(data[i] - predictions[j]);
			if(val < best_val && (predictors & (1 << j))){
				best_val = val;
				best_pred[i % width] = j;
			}
		}
	}
	return out_buf;
}

int layer_encode(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	int cruncher_mode
){
	int possible_size = (depth*size + depth*size % 8)/8;

	uint32_t prob_bits = 15;

	uint32_t* dummy1;
	uint32_t* dummy2;
	uint32_t* dummy3;
	int lz_used = 0;

	uint16_t* predict = channelpredict(data, size, width, height, depth, 0b0000000000010000);
	int channel_size = channel_encode(
		predict,
		size,
		depth + 1,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);
	if(channel_size < possible_size){
		possible_size = channel_size;
	}

	int lz_overhead;
	uint16_t* predict_cleaned;
	size_t cleaned_pointer;
	size_t LEMPEL_SIZE;
	uint8_t* LEMPEL;
	uint8_t* LEMPEL_NUKE;
	int channel_size_lz;
	LEMPEL = new uint8_t[size];
	LEMPEL_NUKE = new uint8_t[size];
	for(int i=0;i<size;i++){
		LEMPEL_NUKE[i] = 0;
	}

	if(cruncher_mode == 0){
		lz_overhead = find_lz(
			data,
			size,
			LEMPEL,
			&LEMPEL_SIZE,
			LEMPEL_NUKE,
			5
		);
	}
	else if(cruncher_mode == 1){
		lz_overhead = find_lz(
			data,
			size,
			LEMPEL,
			&LEMPEL_SIZE,
			LEMPEL_NUKE,
			6
		);
	}
	else if(cruncher_mode == 2){
		lz_overhead = find_lz(
			data,
			size,
			LEMPEL,
			&LEMPEL_SIZE,
			LEMPEL_NUKE,
			8
		);
	}
	else if(cruncher_mode == 3){
		lz_overhead = find_lz(
			data,
			size,
			LEMPEL,
			&LEMPEL_SIZE,
			LEMPEL_NUKE,
			11
		);
	}
	else{
		lz_overhead = find_lz(
			data,
			size,
			LEMPEL,
			&LEMPEL_SIZE,
			LEMPEL_NUKE,
			14
		);
	}

	predict_cleaned = new uint16_t[size];
	cleaned_pointer = 0;
	for(int i=0;i<size;i++){
		if(LEMPEL_NUKE[i] == 0){
			predict_cleaned[cleaned_pointer++] = predict[i];
		}
	}

	channel_size_lz = channel_encode(
		predict_cleaned,
		cleaned_pointer,
		depth + 1,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);

	if(channel_size_lz + lz_overhead < possible_size){
		possible_size = channel_size_lz + lz_overhead;
		lz_used = 1;
	}

	if(cruncher_mode){
		uint32_t mask = 0b0000000000010000;
		int temp_size;

		uint32_t masks[11] = {
			0b0000000000000001,
			0b1111111110111111,//good
			0b0000000000000011,
			0b1111111111111101,
			0b1111111111111011,
			0b1111111111110111,
			0b1111111111101111,
			0b1111111111011111,
			0b1111111101111111,
			0b1111110111111111,

			0b1111111111111111
		};
		for(int i=0;i<11 && i < 1 + cruncher_mode*3;i++){
			predict = channelpredict(data, size, width, height, depth, masks[i]);
			if(lz_used){
				cleaned_pointer = 0;
				for(int j=0;j<size;j++){
					if(LEMPEL_NUKE[j] == 0){
						predict_cleaned[cleaned_pointer++] = predict[j];
					}
				}
				temp_size = channel_encode(
					predict_cleaned,
					cleaned_pointer,
					depth + 1,
					prob_bits,
					dummy1,
					dummy2,
					dummy3
				) + lz_overhead;
			}
			else{
				temp_size = channel_encode(
					predict,
					size,
					depth + 1,
					prob_bits,
					dummy1,
					dummy2,
					dummy3
				);
			}
			if(temp_size < possible_size){
				possible_size = temp_size;
				mask = masks[i];
			}
		}
		/*int best_found = -1;
		for(int i=1;i<16;i++){
			predict = channelpredict(data, size, width, height, depth, 0xFFFF ^ (1 << i));
			if(lz_used){
				cleaned_pointer = 0;
				for(int j=0;j<size;j++){
					if(LEMPEL_NUKE[j] == 0){
						predict_cleaned[cleaned_pointer++] = predict[j];
					}
				}
				temp_size = channel_encode(
					predict_cleaned,
					cleaned_pointer,
					depth + 1,
					prob_bits,
					dummy1,
					dummy2,
					dummy3
				) + lz_overhead;
			}
			else{
				temp_size = channel_encode(
					predict,
					size,
					depth + 1,
					prob_bits,
					dummy1,
					dummy2,
					dummy3
				);
			}
			if(temp_size < possible_size){
				possible_size = temp_size;
				mask = 0xFFFF ^ (1 << i);
				best_found = i;
			}
		}
		if(best_found != -1){
			int best_found2 = -1;
			for(int i=1;i<16;i++){
				if(best_found == i){
					continue;
				}
				predict = channelpredict(data, size, width, height, depth, mask ^ (1 << i));
				if(lz_used){
					cleaned_pointer = 0;
					for(int j=0;j<size;j++){
						if(LEMPEL_NUKE[j] == 0){
							predict_cleaned[cleaned_pointer++] = predict[j];
						}
					}
					temp_size = channel_encode(
						predict_cleaned,
						cleaned_pointer,
						depth + 1,
						prob_bits,
						dummy1,
						dummy2,
						dummy3
					) + lz_overhead;
				}
				else{
					temp_size = channel_encode(
						predict,
						size,
						depth + 1,
						prob_bits,
						dummy1,
						dummy2,
						dummy3
					);
				}
				if(temp_size < possible_size){
					possible_size = temp_size;
					mask = 0xFFFF ^ (1 << i);
					best_found2 = i;
				}
			}
			if(best_found2 != -1){
				for(int i=1;i<16;i++){
					if(best_found == i || best_found2 == i){
						continue;
					}
					predict = channelpredict(data, size, width, height, depth, mask ^ (1 << i));
					if(lz_used){
						cleaned_pointer = 0;
						for(int j=0;j<size;j++){
							if(LEMPEL_NUKE[j] == 0){
								predict_cleaned[cleaned_pointer++] = predict[j];
							}
						}
						temp_size = channel_encode(
							predict_cleaned,
							cleaned_pointer,
							depth + 1,
							prob_bits,
							dummy1,
							dummy2,
							dummy3
						) + lz_overhead;
					}
					else{
						temp_size = channel_encode(
							predict,
							size,
							depth + 1,
							prob_bits,
							dummy1,
							dummy2,
							dummy3
						);
					}
					if(temp_size < possible_size){
						possible_size = temp_size;
						mask = 0xFFFF ^ (1 << i);
					}
				}
			}
		}*/
		//printf("maskend %d\n",(int)mask);
		if(cruncher_mode > 1){
			predict = channelpredict(data, size, width, height, depth, mask);
			cleaned_pointer = 0;
			for(int j=0;j<size;j++){
				if(LEMPEL_NUKE[j] == 0){
					predict_cleaned[cleaned_pointer++] = predict[j];
				}
			}
			if(lz_used){
				int temp_size1 = channel_encode(
					predict_cleaned,
					cleaned_pointer,
					depth + 1,
					16,
					dummy1,
					dummy2,
					dummy3
				) + lz_overhead;
				int temp_size2 = channel_encode(
					predict_cleaned,
					cleaned_pointer,
					depth + 1,
					15,
					dummy1,
					dummy2,
					dummy3
				) + lz_overhead;
				if(temp_size1 < temp_size2){
					if(temp_size1 < possible_size){
						possible_size = temp_size1;
					}
					for(uint32_t i=17;i < 20;i++){
						temp_size = channel_encode(
							predict_cleaned,
							cleaned_pointer,
							depth + 1,
							i,
							dummy1,
							dummy2,
							dummy3
						) + lz_overhead;
						if(temp_size < possible_size){
							possible_size = temp_size;
						}
					}
				}
				else{
					if(temp_size2 < possible_size){
						possible_size = temp_size2;
					}
					for(uint32_t i=14;i > 11;i--){
						temp_size = channel_encode(
							predict_cleaned,
							cleaned_pointer,
							depth + 1,
							i,
							dummy1,
							dummy2,
							dummy3
						) + lz_overhead;
						if(temp_size < possible_size){
							possible_size = temp_size;
						}
					}
				}
			}
			else{
				int temp_size1 = channel_encode(
					predict,
					size,
					depth + 1,
					16,
					dummy1,
					dummy2,
					dummy3
				);
				int temp_size2 = channel_encode(
					predict,
					size,
					depth + 1,
					15,
					dummy1,
					dummy2,
					dummy3
				);
				if(temp_size1 < temp_size2){
					if(temp_size1 < possible_size){
						possible_size = temp_size1;
					}
					for(uint32_t i=17;i < 20;i++){
						temp_size = channel_encode(
							predict,
							size,
							depth + 1,
							i,
							dummy1,
							dummy2,
							dummy3
						);
						if(temp_size < possible_size){
							possible_size = temp_size;
						}
					}
				}
				else{
					if(temp_size2 < possible_size){
						possible_size = temp_size2;
					}
					for(uint32_t i=14;i > 11;i--){
						temp_size = channel_encode(
							predict,
							size,
							depth + 1,
							i,
							dummy1,
							dummy2,
							dummy3
						);
						if(temp_size < possible_size){
							possible_size = temp_size;
						}
					}
				}
			}
		}
	}

	if(cruncher_mode > 0){
		delete[] LEMPEL;
		delete[] LEMPEL_NUKE;
		delete[] predict_cleaned;
	}

	return possible_size + 1;
}

int palette_encode(uint8_t* in_bytes, size_t in_size, int width, int height,int cruncher_mode){
	uint8_t red[256];
	uint8_t green[256];
	uint8_t blue[256];
	int palette_index = 0;

	uint16_t* INDEXED = new uint16_t[in_size/3];

	for(int i=0;i<in_size;i += 3){
		int found = 0;
		for(int j=0;j<palette_index;j++){
			if(
				red[j] == in_bytes[i]
				&& green[j] == in_bytes[i + 1]
				&& blue[j] == in_bytes[i + 2]
			){
				INDEXED[i/3] = j;
				found = 1;
				break;
			}
		}
		if(found == 0){
			red[palette_index] = in_bytes[i];
			green[palette_index] = in_bytes[i + 1];
			blue[palette_index] = in_bytes[i + 2];
			INDEXED[i/3] = palette_index;
			palette_index++;
			if(palette_index == 257){
				delete[] INDEXED;
				return -1;
			}
		}
	}

	uint32_t* pred_buf_1;
	uint32_t* pred_end_1;
	uint32_t* pred_rans_begin_1;
	int encoded_size = layer_encode(
		INDEXED,
		in_size/3,
		width,
		height,
		8,
		cruncher_mode
	) + palette_index*3 + 1;
	delete[] INDEXED;
	return encoded_size;
}

void print_usage(){
	printf("usage: hoh infile.raw width height -sN\n");
	printf("where N is a number 0-3(fast-slow). Default value 1.\n\n");
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
	int cruncher_mode = 1;
	if(argc > 4 && strcmp(argv[4],"-s0") == 0){
		cruncher_mode = 0;
	}
	else if(argc > 4 && strcmp(argv[4],"-s1") == 0){
		cruncher_mode = 1;
	}
	else if(argc > 4 && strcmp(argv[4],"-s2") == 0){
		cruncher_mode = 2;
	}
	else if(argc > 4 && strcmp(argv[4],"-s3") == 0){
		cruncher_mode = 3;
	}
	else if(argc > 4 && strcmp(argv[4],"-s4") == 0){
		cruncher_mode = 4;
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);

	static const uint32_t prob_bits = 16;
	static const uint32_t prob_scale = 1 << prob_bits;

	int best_size;
	if(grey_test(in_bytes, in_size)){
		uint16_t* GREY = channel_picker(in_bytes, in_size, 3, 0);
		best_size = layer_encode(
			GREY,
			in_size/3,
			width,
			height,
			8,
			cruncher_mode
		);
		delete[] GREY;

		uint8_t* binary = channel_picker8(in_bytes, in_size, 3, 0);
		if(binary_test(binary, in_size/3)){
			binarize(binary, in_size/3);

			int bit_size = bitimage_encode(
				binary,
				in_size/3,
				width,
				height
			);
			if(bit_size < best_size){
				best_size = bit_size;
			}
		}
	}
	else{

		int rgb_size = -1;
		int subtract_green_size = 0;
		int palette_size = palette_encode(in_bytes, in_size, width, height, cruncher_mode);

		size_t split_size = in_size/3;
		uint16_t* GREEN  = new uint16_t[split_size];
		uint16_t* RED_G  = new uint16_t[split_size];
		uint16_t* BLUE_G = new uint16_t[split_size];
		subtract_green(in_bytes, in_size, GREEN, RED_G, BLUE_G);

		//palette_compact(GREEN,split_size);//costs 32 bytes, but who's counting?
		int green_size = layer_encode(
			GREEN,
			split_size,
			width,
			height,
			8,
			cruncher_mode
		);
		subtract_green_size += green_size;

		subtract_green_size += layer_encode(
			RED_G,
			split_size,
			width,
			height,
			9,
			cruncher_mode
		);

		subtract_green_size += layer_encode(
			BLUE_G,
			split_size,
			width,
			height,
			9,
			cruncher_mode
		);
		delete[] GREEN;
		delete[] RED_G;
		delete[] BLUE_G;

		if(cruncher_mode > 2){
			uint16_t* RED = channel_picker(in_bytes, in_size, 3, 0);
			uint16_t* BLUE = channel_picker(in_bytes, in_size, 3, 2);
			if(cruncher_mode == 4){
				int red_size = layer_encode(
					RED,
					split_size,
					width,
					height,
					8,
					cruncher_mode
				);
				uint16_t* BLUE = channel_picker(in_bytes, in_size, 3, 2);
				int blue_size = layer_encode(
					BLUE,
					split_size,
					width,
					height,
					8,
					cruncher_mode
				);
				rgb_size = red_size + green_size + blue_size;
			}
			else{
				int red_size = layer_encode(
					RED,
					split_size,
					width,
					height,
					8,
					cruncher_mode - 1
				);
				int blue_size = layer_encode(
					BLUE,
					split_size,
					width,
					height,
					8,
					cruncher_mode - 1
				);
				rgb_size = red_size + green_size + blue_size;
			}
			delete[] RED;
			delete[] BLUE;
		}

		best_size = subtract_green_size;
		if(palette_size != -1 && palette_size < best_size){
			best_size = palette_size;
		}
		if(rgb_size != -1 && rgb_size < best_size){
			best_size = rgb_size;
		}
	}

	printf("%d\n",best_size + 8 + 8 + 2 + 3);
	//printf("Total file size: %d bytes\n",best_size + 8 + 8 + 2 + 3);
	//data size + channel offsets + width/height + transforms + identifier

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
