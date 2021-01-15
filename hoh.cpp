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
#include "lz.hpp"
#include "channel_encode.hpp"
#include "bitimage.hpp"

uint8_t midpoint(uint8_t a, uint8_t b){
	return a + (b - a) / 2;
}

uint16_t midpoint(uint16_t a, uint16_t b){
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

uint16_t* channelpredict_fastpath(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	size_t* buffer_size
){
	int centre = 1<<depth;
	uint16_t* out_buf = new uint16_t[width*height];

	uint16_t top_row[width];
	uint16_t forige;
	uint16_t forige_TL;

	for(int i=0;i<width;i++){
		top_row[i] = centre/2;
	}

	size_t index = 0;
	for(int y_m=0;y_m < height;y_m++){
		forige    = centre/2;
		forige_TL = centre/2;
		for(int x_m=0;x_m < width;x_m++){
			int datalocation = y_m*width + x_m;
			uint16_t L = forige;
			uint16_t T = top_row[x_m];
			uint16_t TL = forige_TL;

			out_buf[index++] = data[datalocation] - median(T,L,T+L-TL) + centre;

			forige_TL = top_row[x_m];
			top_row[x_m] = data[datalocation];
			forige = data[datalocation];
		}
	}
	*buffer_size = index;
	return out_buf;
}

uint16_t* channelpredict_section(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	int x_tiles,
	int y_tiles,
	int x,
	int y,
	uint16_t predictor,
	size_t* buffer_size
){
	if(predictor == 0b0000000000010000 && x_tiles == 1 && y_tiles == 1 && x == 0 && y == 0){
		return channelpredict_fastpath(
			data,
			size,
			width,
			height,
			depth,
			buffer_size
		);
	}
	int centre = 1<<depth;
	int tile_width  = (width  + x_tiles - 1)/x_tiles;
	int tile_height = (height + y_tiles - 1)/y_tiles;
	int x_coord = x*tile_width;
	int y_coord = y*tile_height;
	uint16_t* out_buf = new uint16_t[tile_width*tile_height];

	int best_pred[tile_width];//approximate
	for(int i=0;i<tile_width;i++){
		best_pred[i] = 4;
	}

	uint16_t top_row[tile_width];
	uint16_t forige;
	uint16_t forige_TL;

	if(y){
		for(int i=0;i<tile_width;i++){
			top_row[i] = data[y_coord*width + x_coord + i - width];
		}
	}
	else{
		for(int i=0;i<tile_width;i++){
			top_row[i] = centre/2;
		}
	}
	size_t index = 0;
	for(int y_m=0;y_m < tile_height && y_coord + y_m < height;y_m++){
		if(x){
			forige = data[(y_coord + y_m)*width + x_coord - 1];
			if(y_m || y){
				forige_TL = data[(y_coord + y_m - 1)*width + x_coord - 1];
			}
			else{
				forige_TL = centre/2;
			}
		}
		else{
			forige    = centre/2;
			forige_TL = centre/2;
		}
		for(int x_m=0;x_m < tile_width && x_coord + x_m < width;x_m++){
			int datalocation = (y_coord + y_m)*width + x_coord + x_m;
			uint16_t L = forige;
			uint16_t T = top_row[x_m];
			uint16_t TL = forige_TL;
			uint16_t TR = top_row[(x_m + tile_width + 1) % tile_width];
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
			};
			out_buf[index++] = data[datalocation] - midpoint(predictions[best_pred[x_m]],predictions[best_pred[(x_m + tile_width - 1) % tile_width]]) + centre;
			forige_TL = top_row[x_m];
			top_row[x_m] = data[datalocation];
			forige = data[datalocation];
			int best_val = centre*2;
			best_pred[x_m] = 0;
			for(int j=0;j<16;j++){
				int val = std::abs(data[datalocation] - predictions[j]);
				if(val < best_val && (predictor & (1 << j))){
					best_val = val;
					best_pred[x_m] = j;
				}
			}
		}
	}
	*buffer_size = index;
	return out_buf;
}

uint16_t* channelpredict_all(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	int x_tiles,
	int y_tiles,
	uint16_t* tile_map
){
	int centre = 1<<depth;
	int tile_width  = (width  + x_tiles - 1)/x_tiles;
	int tile_height = (height + y_tiles - 1)/y_tiles;
	uint16_t* out_buf = new uint16_t[size];

	int best_pred[width];
	for(int i=0;i<width;i++){
		best_pred[i] = 4;
	}

	uint16_t top_row[width];
	uint16_t forige;
	uint16_t forige_TL;
	for(int i=0;i<width;i++){
		top_row[i] = centre/2;
	}
	size_t index = 0;
	for(int y_m=0;y_m < height;y_m++){
		forige    = centre/2;
		forige_TL = centre/2;
		for(int x_m=0;x_m < width;x_m++){
			int datalocation = y_m*width + x_m;
			uint16_t prediction;
			uint16_t predictor = tile_map[(y_m/tile_height) * x_tiles + (x_m/tile_width)];
			uint16_t L = forige;
			uint16_t T = top_row[x_m];
			uint16_t TL = forige_TL;
			uint16_t TR = top_row[(x_m + width + 1) % width];
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
			};
			out_buf[index++] = data[datalocation] - midpoint(predictions[best_pred[x_m]],predictions[best_pred[(x_m + width - 1) % width]]) + centre;
			forige_TL = top_row[x_m];
			top_row[x_m] = data[datalocation];
			forige = data[datalocation];

			int best_val = centre*2;
			best_pred[x_m] = 0;

			if(y_m + 1 < height){
				predictor = tile_map[((y_m + 1)/tile_height) * x_tiles + (x_m/tile_width)];//future row
				for(int j=0;j<16;j++){
					int val = std::abs(data[datalocation] - predictions[j]);
					if(val < best_val && (predictor & (1 << j))){
						best_val = val;
						best_pred[x_m] = j;
					}
				}
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
	int cruncher_mode,
	uint8_t* LEMPEL_NUKE
){
	int possible_size = (depth*size + depth*size % 8)/8;

	uint32_t prob_bits = 15;

	uint32_t* dummy1;
	uint32_t* dummy2;
	uint32_t* dummy3;

	size_t chunk_size;

	uint16_t* predict = channelpredict_section(
		data,
		size,
		width,
		height,
		depth,
		1,
		1,
		0,
		0,
		0b0000000000010000,
		&chunk_size
	);

	uint16_t* predict_cleaned;
	size_t cleaned_pointer;

	predict_cleaned = new uint16_t[size];
	cleaned_pointer = 0;
	for(int i=0;i<size;i++){
		if(LEMPEL_NUKE[i] == 0){
			predict_cleaned[cleaned_pointer++] = predict[i];
		}
	}

	int channel_size_lz = channel_encode(
		predict_cleaned,
		cleaned_pointer,
		depth + 1,
		prob_bits,
		dummy1,
		dummy2,
		dummy3
	);

	if(channel_size_lz < possible_size){
		possible_size = channel_size_lz;
	}

	int total_tiles = 1;
	int tile_cost = 2;
	if(cruncher_mode){
		int freqs[1<<(depth + 1)];
		for(int i=0;i < (1<<(depth + 1));i++){
			freqs[i] = 1;//to have no zero values, so the frequency table is usable for other predictors
		}
		for(int i=0;i<size;i++){
			freqs[predict[i]]++;
		}
		/*for(int i=0;i < (1<<(depth + 1));i++){
			printf("f %d\n",freqs[i]);
		}*/
		double entropy[1<<(depth + 1)];
		for(int i=0;i < (1<<(depth + 1));i++){
			entropy[i] = -std::log2((double)freqs[i]/(double)size);
		}
		/*for(int i=0;i < (1<<(depth + 1));i++){
			printf("f %f\n",entropy[i]);
		}*/
		double total_entropy = 0;
		for(int i=0;i < (1<<(depth + 1));i++){
			total_entropy += entropy[i] * (freqs[i] - 1);
		}
		//printf("total entropy 1 %f\n",total_entropy/8);

		int grid_size = 40;

		int x_tiles = (width + grid_size - 1)/grid_size;
		int y_tiles = (height + grid_size - 1)/grid_size;
		total_tiles = x_tiles*y_tiles;
		uint16_t* predictor_list = new uint16_t[total_tiles];
		double new_cost = 0;

		uint16_t masks[14] = {
			0b0000000000000001,
			0b0000000000000010,
			0b0000000000100000,
			0b0000000000010000,
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
		for(int i=0;i<total_tiles;i++){
			double tile_cost = 99999999999;//yea, fix this later
			for(int pred=0;pred<14 && pred < cruncher_mode*5;pred++){
				uint16_t* tile_predict = channelpredict_section(
					data,
					size,
					width,
					height,
					depth,
					x_tiles,
					y_tiles,
					i % x_tiles,
					i / x_tiles,
					masks[pred],
					&chunk_size
				);
				double current_cost = 0;
				for(int val=0;val<chunk_size;val++){
					current_cost += entropy[tile_predict[val]];
				}
				if(current_cost < tile_cost){
					tile_cost = current_cost;
					predictor_list[i] = masks[pred];
				}
			}
			new_cost += tile_cost;
		}
		//printf("total entropy 2 %f\n",new_cost/8);
		for(int i=0;i<total_tiles;i++){
			//printf("preds %d\n",predictor_list[i]);
		}

		predict = channelpredict_all(
			data,
			size,
			width,
			height,
			depth,
			x_tiles,
			y_tiles,
			predictor_list
		);
		if(cruncher_mode > 2){//refine estimate

			for(int i=0;i < (1<<(depth + 1));i++){
				freqs[i] = 1;
			}
			for(int i=0;i<size;i++){
				freqs[predict[i]]++;
			}
			for(int i=0;i < (1<<(depth + 1));i++){
				entropy[i] = -std::log2((double)freqs[i]/(double)size);
			}
			total_entropy = 0;
			for(int i=0;i < (1<<(depth + 1));i++){
				total_entropy += entropy[i] * (freqs[i] - 1);
			}
			//printf("total entropy 3 %f\n",total_entropy/8);

			new_cost = 0;
			for(int i=0;i<total_tiles;i++){
				double tile_cost = 99999999999;//yea, fix this later
				for(int pred=0;pred<14 && pred < cruncher_mode*5;pred++){
					uint16_t* tile_predict = channelpredict_section(
						data,
						size,
						width,
						height,
						depth,
						x_tiles,
						y_tiles,
						i % x_tiles,
						i / x_tiles,
						masks[pred],
						&chunk_size
					);
					double current_cost = 0;
					for(int val=0;val<chunk_size;val++){
						current_cost += entropy[tile_predict[val]];
					}
					if(current_cost < tile_cost){
						tile_cost = current_cost;
						predictor_list[i] = masks[pred];
					}
				}
				new_cost += tile_cost;
			}
			//printf("total entropy 4 %f\n",new_cost/8);
		}
		//printf("tiles %d\n",total_tiles);

		tile_cost = 4 + (total_tiles)/2;

		delete[] predictor_list;
	}
	if(cruncher_mode){
		int temp_size;
		cleaned_pointer = 0;
		for(int j=0;j<size;j++){
			if(LEMPEL_NUKE[j] == 0){
				predict_cleaned[cleaned_pointer++] = predict[j];
			}
		}
		int temp_size1 = channel_encode(
			predict_cleaned,
			cleaned_pointer,
			depth + 1,
			16,
			dummy1,
			dummy2,
			dummy3
		);
		int temp_size2 = channel_encode(
			predict_cleaned,
			cleaned_pointer,
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
					predict_cleaned,
					cleaned_pointer,
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
					predict_cleaned,
					cleaned_pointer,
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

	delete[] predict;
	delete[] predict_cleaned;

	return possible_size + 1 + tile_cost;
}

int count_colours(uint8_t* in_bytes, size_t in_size){
	uint8_t red[256];
	uint8_t green[256];
	uint8_t blue[256];
	int palette_index = 0;

	for(int i=0;i<in_size;i += 3){
		int found = 0;
		for(int j=0;j<palette_index;j++){
			if(
				red[j] == in_bytes[i]
				&& green[j] == in_bytes[i + 1]
				&& blue[j] == in_bytes[i + 2]
			){
				found = 1;
				break;
			}
		}
		if(found == 0){
			red[palette_index] = in_bytes[i];
			green[palette_index] = in_bytes[i + 1];
			blue[palette_index] = in_bytes[i + 2];
			palette_index++;
			if(palette_index == 257){
				return -1;
			}
		}
	}
	return palette_index;
}

int palette_encode(uint8_t* in_bytes, size_t in_size, int width, int height,int cruncher_mode, uint8_t* LEMPEL_NUKE){
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
		cruncher_mode,
		LEMPEL_NUKE
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

	uint8_t* LEMPEL = new uint8_t[in_size/3];
	uint8_t* LEMPEL_NUKE = new uint8_t[in_size/3];
	for(int i=0;i<in_size/3;i++){
		LEMPEL_NUKE[i] = 0;
	}
	size_t lz_symbol_size;

	int seek_distance = 6;
	if(cruncher_mode == 1){
		seek_distance = 10;
	}
	else if(cruncher_mode == 2){
		seek_distance = 11;
	}
	else if(cruncher_mode == 3){
		seek_distance = 12;
	}
	else if(cruncher_mode == 4){
		seek_distance = 14;
	}

	int break_even_addition = 0;
	int colour_count = count_colours(in_bytes, in_size);
	if(colour_count != -1){
		if(colour_count <= 4){
			break_even_addition = 32;
		}
		else if(colour_count <= 8){
			break_even_addition = 20;
		}
		else if(colour_count <= 16){
			break_even_addition = 10;
		}
		else if(colour_count <= 32){
			break_even_addition = 2;
		}
	}

	int lz_external_overhead = find_lz_rgb(
		in_bytes,
		in_size,
		LEMPEL,
		&lz_symbol_size,
		LEMPEL_NUKE,
		seek_distance,
		break_even_addition
	);

	int best_size;
	if(grey_test(in_bytes, in_size)){
		uint16_t* GREY = channel_picker(in_bytes, in_size, 3, 0);
		best_size = layer_encode(
			GREY,
			in_size/3,
			width,
			height,
			8,
			cruncher_mode,
			LEMPEL_NUKE
		) + lz_external_overhead;
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
		int palette_size = palette_encode(in_bytes, in_size, width, height, cruncher_mode, LEMPEL_NUKE);

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
			cruncher_mode,
			LEMPEL_NUKE
		);
		subtract_green_size += green_size;

		subtract_green_size += layer_encode(
			RED_G,
			split_size,
			width,
			height,
			9,
			cruncher_mode,
			LEMPEL_NUKE
		);

		subtract_green_size += layer_encode(
			BLUE_G,
			split_size,
			width,
			height,
			9,
			cruncher_mode,
			LEMPEL_NUKE
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
					cruncher_mode,
					LEMPEL_NUKE
				);
				uint16_t* BLUE = channel_picker(in_bytes, in_size, 3, 2);
				int blue_size = layer_encode(
					BLUE,
					split_size,
					width,
					height,
					8,
					cruncher_mode,
					LEMPEL_NUKE
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
					cruncher_mode - 1,
					LEMPEL_NUKE
				);
				int blue_size = layer_encode(
					BLUE,
					split_size,
					width,
					height,
					8,
					cruncher_mode - 1,
					LEMPEL_NUKE
				);
				rgb_size = red_size + green_size + blue_size;
			}
			delete[] RED;
			delete[] BLUE;
		}

		best_size = subtract_green_size + lz_external_overhead;
		if(palette_size != -1 && palette_size + lz_external_overhead < best_size){
			best_size = palette_size + lz_external_overhead;
		}
		if(rgb_size != -1 && rgb_size + lz_external_overhead < best_size){
			best_size = rgb_size + lz_external_overhead;
		}
	}

	delete[] LEMPEL;
	delete[] LEMPEL_NUKE;

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
