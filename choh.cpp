#include "platform.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <cstdlib>

#include "varint.hpp"
#include "file_io.hpp"
#include "channel.hpp"
#include "lz.hpp"
#include "bitimage.hpp"
#include "layer_encode.hpp"

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

int palette_encode(
	uint8_t* in_bytes,
	size_t in_size,
	int width,
	int height,
	int cruncher_mode,
	uint8_t* LEMPEL_NUKE,
	uint8_t* compressed
){
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
		LEMPEL_NUKE,
		compressed
	) + palette_index*3 + 1;
	delete[] INDEXED;
	return encoded_size;
}

size_t encode_tile(
	uint8_t* in_bytes,
	size_t in_size,
	uint8_t* out_buf,
	int width,
	int height,
	int cruncher_mode
){
	size_t out_max_size = in_size + 256;//safety margin
	size_t out_start = 0;

	// 1x1 tile image
	out_buf[out_start++] = 0;
	out_buf[out_start++] = 0;
	//no offsets needed

	static const uint32_t prob_bits = 16;
	static const uint32_t prob_scale = 1 << prob_bits;

	uint8_t* LEMPEL = new uint8_t[in_size/3];
	uint8_t* LEMPEL_NUKE = new uint8_t[in_size/3];
	for(int i=0;i<in_size/3;i++){
		LEMPEL_NUKE[i] = 0;
	}

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

	size_t lz_external_overhead = find_lz_rgb(
		in_bytes,
		in_size,
		width,
		height,
		LEMPEL,
		LEMPEL_NUKE,
		seek_distance,
		break_even_addition
	);

	int best_size;
	int internal_colour_mode = 2;//rgb default
	uint8_t channel_number = 3;

	size_t channel_size1;
	size_t channel_size2;
	size_t channel_size3;

	uint8_t* channel_compressed1 = new uint8_t[in_size + 256];
	uint8_t* channel_compressed2;
	uint8_t* channel_compressed3;

	if(grey_test(in_bytes, in_size)){
		channel_number = 1;
		uint8_t* binary = channel_picker8(in_bytes, in_size, 3, 0);
		if(binary_test(binary, in_size/3)){
			binarize(binary, in_size/3);

			int bit_size = bitimage_encode(
				binary,
				in_size/3,
				width,
				height
			);
			best_size = bit_size;
			internal_colour_mode = 0;//bitimage
		}
		else{
			//TODO 8bit for less memory usage
			uint16_t* GREY = channel_picker(in_bytes, in_size, 3, 0);
			channel_size1 = layer_encode(
				GREY,
				in_size/3,
				width,
				height,
				8,
				cruncher_mode,
				LEMPEL_NUKE,
				channel_compressed1
			) + lz_external_overhead;
			internal_colour_mode = 1;//greyscale
			delete[] GREY;
		}
	}
	else{

		int rgb_size = -1;
		int subtract_green_size = 0;

		size_t split_size = in_size/3;
		uint16_t* GREEN  = new uint16_t[split_size];
		uint16_t* RED_G  = new uint16_t[split_size];
		uint16_t* BLUE_G = new uint16_t[split_size];
		subtract_green(in_bytes, in_size, GREEN, RED_G, BLUE_G);

		channel_size1 = layer_encode(
			GREEN,
			split_size,
			width,
			height,
			8,
			cruncher_mode,
			LEMPEL_NUKE,
			channel_compressed1
		);

		channel_compressed2 = new uint8_t[in_size + 256];
		channel_compressed3 = new uint8_t[in_size + 256];

		channel_size2 = layer_encode(
			RED_G,
			split_size,
			width,
			height,
			9,
			cruncher_mode,
			LEMPEL_NUKE,
			channel_compressed2
		);

		channel_size3 = layer_encode(
			BLUE_G,
			split_size,
			width,
			height,
			9,
			cruncher_mode,
			LEMPEL_NUKE,
			channel_compressed3
		);
		delete[] GREEN;
		delete[] RED_G;
		delete[] BLUE_G;
		subtract_green_size += channel_size1 + channel_size2 + channel_size3;

		size_t red_channel_size;
		size_t blue_channel_size;
		uint8_t* alt_comp2;
		uint8_t* alt_comp3;
		if(cruncher_mode > 2){
			alt_comp2 = new uint8_t[in_size + 256];
			alt_comp3 = new uint8_t[in_size + 256];
			uint16_t* RED = channel_picker(in_bytes, in_size, 3, 0);
			red_channel_size = layer_encode(
				RED,
				split_size,
				width,
				height,
				8,
				cruncher_mode,
				LEMPEL_NUKE,
				alt_comp2
			);
			delete[] RED;
			uint16_t* BLUE = channel_picker(in_bytes, in_size, 3, 2);
			blue_channel_size = layer_encode(
				BLUE,
				split_size,
				width,
				height,
				8,
				cruncher_mode,
				LEMPEL_NUKE,
				alt_comp3
			);
			rgb_size = red_channel_size + channel_size1 + blue_channel_size;
			delete[] BLUE;
		}

		best_size = subtract_green_size + lz_external_overhead;
		internal_colour_mode = 128;//sub_green

		uint8_t* channel_compressed_indexed = new uint8_t[in_size + 256];
		int palette_size = palette_encode(in_bytes, in_size, width, height, cruncher_mode, LEMPEL_NUKE, channel_compressed_indexed);

		if(palette_size != -1 && palette_size + lz_external_overhead < best_size){
			best_size = palette_size + lz_external_overhead;
			internal_colour_mode = 127;//indexed
			channel_number = 1;
			uint8_t* tmp = channel_compressed1;
			channel_compressed1 = channel_compressed_indexed;
			channel_compressed_indexed = tmp;
		}
		if(rgb_size != -1 && rgb_size + lz_external_overhead < best_size){
			best_size = rgb_size + lz_external_overhead;
			internal_colour_mode = 2;//rgb
			channel_number = 3;
			channel_size2 = red_channel_size;
			channel_size3 = blue_channel_size;

			uint8_t* tmp = channel_compressed2;
			channel_compressed2 = alt_comp2;
			alt_comp2 = tmp;
			tmp = channel_compressed3;
			channel_compressed3 = alt_comp3;
			alt_comp3 = tmp;

			delete[] alt_comp2;
			delete[] alt_comp3;
		}
		delete[] channel_compressed_indexed;
	}
	out_buf[out_start++] = internal_colour_mode;
	for(size_t i=0;i<lz_external_overhead;i++){
		out_buf[out_start++] = LEMPEL[i];
	}
	delete[] LEMPEL;

	//never reorder
	if(channel_number == 1){
		for(size_t i=0;i<channel_size1;i++){
			out_buf[out_start++] = channel_compressed1[i];
		}
	}
	else if(channel_number == 2){
		out_buf[out_start++] = 0b00000100;//xx xx 1 0
		write_varint(out_buf, &out_start, channel_size1);
		for(size_t i=0;i<channel_size1;i++){
			out_buf[out_start++] = channel_compressed1[i];
		}
		for(size_t i=0;i<channel_size2;i++){
			out_buf[out_start++] = channel_compressed2[i];
		}
		delete[] channel_compressed2;
	}
	else if(channel_number == 3){
		out_buf[out_start++] = 0b00100100;//xx 2 1 0
		write_varint(out_buf, &out_start, channel_size1);
		write_varint(out_buf, &out_start, channel_size2);
		for(size_t i=0;i<channel_size1;i++){
			out_buf[out_start++] = channel_compressed1[i];
		}
		for(size_t i=0;i<channel_size2;i++){
			out_buf[out_start++] = channel_compressed2[i];
		}
		for(size_t i=0;i<channel_size3;i++){
			out_buf[out_start++] = channel_compressed3[i];
		}

		delete[] channel_compressed2;
		delete[] channel_compressed3;
	}
	else if(channel_number == 4){
		printf("unimplemented channel number! %d\n",(int)channel_number);
		out_buf[out_start++] = 0b11100100;//3 2 1 0
		write_varint(out_buf, &out_start, channel_size1);
		write_varint(out_buf, &out_start, channel_size2);
		write_varint(out_buf, &out_start, channel_size3);
	}
	else{
		printf("unimplemented channel number! %d\n",(int)channel_number);
		out_buf[out_start++] = 0;
	}

	delete[] channel_compressed1;
	delete[] LEMPEL_NUKE;

	return out_start;
}

void print_usage(){
	printf("usage: choh infile.rgb outfile.hoh width height -sN\n");
	printf("where N is a number 0-3(fast-slow). Default value 1.\n\n");
	printf("The input file must consist of raw 8bit RGB bytes\n");
	printf("You can make such a file with imagemagick:\n");
	printf("convert input.png -depth 8 rgb:infile.rgb\n\n");
	printf("Output will at present incomplete because file IO is tedious\n");
}

int main(int argc, char *argv[]){
	if(argc < 5){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	int width = atoi(argv[3]);
	int height = atoi(argv[4]);
	if(width == 0 || height == 0){
		printf("invalid width or height\n");
		print_usage();
		return 2;
	}
	int cruncher_mode = 1;
	if(argc > 4 && strcmp(argv[5],"-s0") == 0){
		cruncher_mode = 0;
	}
	else if(argc > 4 && strcmp(argv[5],"-s1") == 0){
		cruncher_mode = 1;
	}
	else if(argc > 4 && strcmp(argv[5],"-s2") == 0){
		cruncher_mode = 2;
	}
	else if(argc > 4 && strcmp(argv[5],"-s3") == 0){
		cruncher_mode = 3;
	}
	else if(argc > 4 && strcmp(argv[5],"-s4") == 0){
		cruncher_mode = 4;
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);

	size_t out_max_size = in_size + 256;//safety margin
	uint8_t* out_buf = new uint8_t[out_max_size];
	size_t out_start = 0;

	//mandatory header:
	out_buf[out_start++] = 153;
	out_buf[out_start++] = 72;
	out_buf[out_start++] = 79;
	out_buf[out_start++] = 72;

	//color format, encoder only takes rgb :)
	out_buf[out_start++] = 2;

	//only 8bit image for now
	out_buf[out_start++] = 8;

	//width, height, varints
	write_varint(out_buf, &out_start, width - 1);
	write_varint(out_buf, &out_start, height - 1);
	//end header
	size_t tile_size = 0;

	if((width >= 512 || height >= 512) && width >= 256 && height >= 256){
		int x_tiles = (width ) / 256;
		int y_tiles = (height) / 256;
		out_buf[out_start++] = x_tiles - 1;
		out_buf[out_start++] = y_tiles - 1;
		int tile_width  = (width  + x_tiles - 1)/x_tiles;
		int tile_height = (height + y_tiles - 1)/y_tiles;

		uint8_t* tile_queue[x_tiles*y_tiles];
		size_t tile_queue_sizes[x_tiles*y_tiles];
		for(int i=0;i<x_tiles*y_tiles;i++){
			int new_width = tile_width;
			int new_height = tile_height;
			int x_offset = (i % x_tiles)*tile_width;
			int y_offset = (i / x_tiles)*tile_height;
			if(width - x_offset < new_width){
				new_width = width - x_offset;
			}
			if(height - y_offset < new_height){
				new_height = height - y_offset;
			}
			size_t new_size = new_width*new_height;
			uint8_t* new_bytes = new uint8_t[new_size*3];

			for(int y=0;y<new_height;y++){
				for(int x=0;x<new_width;x++){
					new_bytes[(y*new_width + x)*3]     = in_bytes[((y + y_offset)*width + x_offset + x)*3];
					new_bytes[(y*new_width + x)*3 + 1] = in_bytes[((y + y_offset)*width + x_offset + x)*3 + 1];
					new_bytes[(y*new_width + x)*3 + 2] = in_bytes[((y + y_offset)*width + x_offset + x)*3 + 2];
				}
			}
			size_t out_max_size = (new_size*3) + 256;//safety margin
			uint8_t* written_location = new uint8_t[out_max_size];
			tile_queue_sizes[i] = encode_tile(
				new_bytes,
				new_size*3,
				written_location,
				new_width,
				new_height,
				cruncher_mode
			);
			tile_queue[i] = written_location;
			if(i+1 != x_tiles*y_tiles){
				write_varint(out_buf, &out_start, tile_queue_sizes[i]);
			}
			delete[] new_bytes;
		}
		for(int i=0;i<x_tiles*y_tiles;i++){
			for(size_t j=0;j<tile_queue_sizes[i];j++){//TODO pass actual data
				out_buf[out_start++] = tile_queue[i][j];
			}
			delete tile_queue[i];
		}
	}
	else{
		uint8_t* tile_buf;
		tile_size += encode_tile(
			in_bytes,
			in_size,
			tile_buf,
			width,
			height,
			cruncher_mode
		);
		delete[] tile_buf;
	}

	printf("%d\n",(int)(out_start + tile_size));

	delete[] in_bytes;
	write_file(argv[2],out_buf,out_start);
	return 0;
}
