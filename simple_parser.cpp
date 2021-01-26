#include "platform.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "varint.hpp"
#include "file_io.hpp"
#include "layer_decode.hpp"
#include "un_lz.hpp"

/*
This is intended as a more stupid version of dhoh, which does not do any actual decoding.
Instead, it does the bare minimum that's needed to print the structure of the file, or crash if something's wrong.
It's a semi-independent implementation to help catch bugs in choh and dhoh
*/

void decode_tile_simple(
	uint8_t* in_bytes,
	size_t in_size,
	size_t byte_pointer,
	uint8_t pixel_format,
	uint8_t channel_number,
	uint8_t bit_depth,
	size_t width,
	size_t height
){
	uint8_t x_tiles = in_bytes[byte_pointer++] + 1;
	uint8_t y_tiles = in_bytes[byte_pointer++] + 1;

	if(x_tiles != 1 || y_tiles != 1){
		printf("[SIMPLE] tiled image: %dx%d (%d tiles)\n",(int)x_tiles,(int)y_tiles,(int)x_tiles * (int)y_tiles);
		size_t offsets[x_tiles*y_tiles];
		offsets[0] = 0;
		printf("[SIMPLE]   tile offset 0: 0\n");
		for(size_t i=1;i<x_tiles*y_tiles;i++){
			offsets[i] = read_varint(in_bytes, &byte_pointer);
			printf("[SIMPLE]   tile offset %d: %d\n",(int)i,(int)offsets[i]);
		}

		int tile_width = (width + x_tiles - 1)/x_tiles;
		int tile_height = (height + y_tiles - 1)/y_tiles;
		int cummulative_offset = 0;
		for(int i=0;i<x_tiles*y_tiles;i++){
			cummulative_offset += offsets[i];
			size_t x_offset = (i % x_tiles) * tile_width;
			size_t y_offset = (i / x_tiles) * tile_height;
			size_t new_width = tile_width;
			size_t new_height = tile_height;
			if(width - x_offset < new_width){
				new_width = width - x_offset;
			}
			if(height - y_offset < new_height){
				new_height = height - y_offset;
			}
			decode_tile_simple(
				in_bytes,
				in_size,
				byte_pointer + cummulative_offset,
				pixel_format,
				channel_number,
				bit_depth,
				new_width,
				new_height
			);
		}
		return;
	}
	else{
		printf("[SIMPLE] single tile %dx%d\n",(int)width,(int)height);
	}
	if(bit_depth > 8){
		printf("[SIMPLE] high bit depth not implemented!\n");
	}
	uint8_t pixel_format_internal = in_bytes[byte_pointer++];
	uint8_t channel_number_internal = 1;
	if(pixel_format == 0){
		printf("[SIMPLE]   internal pixel format: bit\n");
	}
	else if(pixel_format_internal == 1){
		printf("[SIMPLE]   internal pixel format: greyscale\n");
	}
	else if(pixel_format_internal == 2){
		printf("[SIMPLE]   internal pixel format: rgb\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 3){
		printf("[SIMPLE]   internal pixel format: greyscale + alpha\n");
		channel_number_internal = 2;
	}
	else if(pixel_format_internal == 4){
		printf("[SIMPLE]   internal pixel format: rgb + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 127){
		printf("[SIMPLE]   internal pixel format: indexed\n");
		int index_size = read_varint(in_bytes, &byte_pointer);
		printf("[SIMPLE]   index length: %d\n",index_size);
	}
	else if(pixel_format_internal == 128){
		printf("[SIMPLE]   internal pixel format: subgreen\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 129){
		printf("[SIMPLE]   internal pixel format: YIQ\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 130){
		printf("[SIMPLE]   internal pixel format: subgreen + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 131){
		printf("[SIMPLE]   internal pixel format: YIQ + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 255){
		printf("[SIMPLE]   invalid internal pixel format!\n");
	}
	else{
		printf("[SIMPLE]   unknown pixel format %d!\n",(int)pixel_format_internal);
	}
	
	un_lz_simple(
		in_bytes,
		in_size,
		&byte_pointer,
		width,
		height
	);

	if(channel_number_internal == 1){
		//no need for additional channel information
		decode_layer_simple(
			in_bytes,
			in_size,
			byte_pointer,
			width,
			height,
			bit_depth
		);
	}
	else if(channel_number_internal == 2){
		uint8_t channel_reordering = in_bytes[byte_pointer++];
		uint8_t bit1 = channel_reordering & 1;
		uint8_t bit2 = channel_reordering & 2;
		size_t offset = 0;
		if(bit1 == 0 && bit2 == 0){
			//conjoined
			printf("[SIMPLE]   conjoined channels\n");
		}
		else if(bit1 == 0 && bit2 == 1){
			//regular order
			offset = read_varint(in_bytes, &byte_pointer);
			printf("[SIMPLE]   regular channel layout\n");
		}
		else if(bit1 == 1 && bit2 == 0){
			//inverse order
			offset = read_varint(in_bytes, &byte_pointer);
			printf("[SIMPLE]   inverse channel layout\n");
		}
		else{
			//illegal
			printf("[SIMPLE]   illegal channel layout!\n");
		}

		decode_layer_simple(
			in_bytes,
			in_size,
			byte_pointer,
			width,
			height,
			bit_depth
		);
		decode_layer_simple(
			in_bytes,
			in_size,
			byte_pointer + offset,
			width,
			height,
			bit_depth
		);
	}
	else if(channel_number_internal == 3){
		uint8_t channel_reordering = in_bytes[byte_pointer++];
		uint8_t bit1 = channel_reordering & 0b00000011;
		uint8_t bit2 = (channel_reordering & 0b00001100) >> 2;
		uint8_t bit3 = (channel_reordering & 0b00110000) >> 4;
		int offset1;
		int offset2;
		if(bit1 == 0 && bit2 == 0 && bit3 == 0){
			//conjoined (consider using greyscale internally)
			printf("[SIMPLE]   conjoined channels\n");
			offset1 = 0;
			offset2 = 0;
		}
		else if(bit1 != bit2 && bit1 != bit3){
			offset1 = read_varint(in_bytes, &byte_pointer);
			offset2 = read_varint(in_bytes, &byte_pointer);
			printf("[SIMPLE]   channel order: %d %d %d\n",(int)bit1,(int)bit2,(int)bit3);
		}
		else{
			offset1 = read_varint(in_bytes, &byte_pointer);
			offset2 = 0;
			printf("[SIMPLE]   semi-conjoined channels\n");
		}

		//conjoined channels not decoded
		decode_layer_simple(
			in_bytes,
			in_size,
			byte_pointer,
			width,
			height,
			bit_depth
		);
		decode_layer_simple(
			in_bytes,
			in_size,
			byte_pointer + offset1,
			width,
			height,
			bit_depth
		);
		decode_layer_simple(
			in_bytes,
			in_size,
			byte_pointer + offset1 + offset2,
			width,
			height,
			bit_depth
		);
	}
	else if(channel_number_internal == 4){
	}
}

void print_usage(){
	printf("usage: simple_parser infile.hoh\n");
}

int main(int argc, char *argv[]){
	if(argc < 2){
		printf("not enough arguments!\n");
		print_usage();
		return 1;
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);

	size_t byte_pointer = 0;
	if(
		in_bytes[byte_pointer++] != 153
		|| in_bytes[byte_pointer++] != 72
		|| in_bytes[byte_pointer++] != 79
		|| in_bytes[byte_pointer++] != 72
	){
		printf("not a valid hoh file!\n");
		return 4;
	}

	uint8_t pixel_format = in_bytes[byte_pointer++];
	uint8_t channel_number = 1;
	if(pixel_format == 0){
		printf("pixel format: bit\n");
	}
	else if(pixel_format == 1){
		printf("pixel format: greyscale\n");
	}
	else if(pixel_format == 2){
		printf("pixel format: rgb\n");
		channel_number = 3;
	}
	else if(pixel_format == 3){
		printf("pixel format: greyscale + alpha\n");
		channel_number = 2;
	}
	else if(pixel_format == 4){
		printf("pixel format: rgb + alpha\n");
		channel_number = 4;
	}
	else if(pixel_format == 255){
		printf("invalid pixel format!\n");
		return 4;
	}
	else{
		printf("unknown pixel format!\n");
		return 5;
	}
	uint8_t bit_depth = in_bytes[byte_pointer++];
	if(pixel_format == 0 && !(bit_depth == 0 || bit_depth == 1)){
		printf("bit depth is too high for a bit image!\n");
		return 4;
	}
	printf("bit depth: %d\n",(int)bit_depth);
	size_t width = read_varint(in_bytes, &byte_pointer) + 1;
	size_t height = read_varint(in_bytes, &byte_pointer) + 1;
	printf("dimensions: %dx%d\n",(int)width,(int)height);
	printf("compressed size: %d bytes (%.3f%% or raw RGB)\n",(int)in_size,(double)(100*in_size)/(double)(width*height*3));

	decode_tile_simple(
		in_bytes,
		in_size,
		byte_pointer,
		pixel_format,
		channel_number,
		bit_depth,
		width,
		height
	);
	delete[] in_bytes;

	return 0;
}
