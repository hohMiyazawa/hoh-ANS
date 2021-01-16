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

void print_usage(){
	printf("usage: dhoh infile.hoh outfile.rgb\n");
	printf("The output file consist of raw 8bit RGB bytes\n");
	printf("You can turn that into an image with imagemagick:\n");
	printf("[TODO: imagemagick command]\n");
}
/*
	return values:
	0: OK
	1: CLI error
	2: I/O error
	3: unexpected end of file
	4: incorrect bitstream
	5: not implemented
	6: resources exhausted
	7: terminated before completion
	255: unknown error
*/
int read_varint(uint8_t* bytes, int* location){//incorrect implentation! only works up to 16383!
	uint8_t first_byte = bytes[*location];
	*location = *location + 1;
	if(first_byte & (1<<7)){
		uint8_t second_byte = bytes[*location++];
		*location = *location + 1;
		return (((int)first_byte & 0b01111111) << 7) + second_byte;
	}
	else{
		return (int)first_byte;
	}
}

uint8_t* decode_tile(
	uint8_t* in_bytes,
	size_t in_size,
	int byte_pointer,
	uint8_t pixel_format,
	uint8_t bit_depth,
	int width,
	int height
){
	uint8_t x_tiles = in_bytes[byte_pointer++] + 1;
	uint8_t y_tiles = in_bytes[byte_pointer++] + 1;

	uint8_t* decoded;

	if(x_tiles != 1 || y_tiles != 1){
		printf("tiled image: %dx%d\n",(int)x_tiles,(int)y_tiles);
		printf("tile offset 0: 0\n");
		for(int i=1;i<x_tiles*y_tiles;i++){
			printf("tile offset %d: %d\n",i,read_varint(in_bytes, &byte_pointer));
		}
	}
	else{
		printf("single tile image\n");
	}
	if(bit_depth > 8){
		printf("high bit depth not implemented!\n");
	}
	uint8_t pixel_format_internal = in_bytes[byte_pointer++];
	if(pixel_format == 0){
		printf("internal pixel format: bit\n");
	}
	else if(pixel_format_internal == 1){
		printf("internal pixel format: greyscale\n");
	}
	else if(pixel_format_internal == 2){
		printf("internal pixel format: rgb\n");
	}
	else if(pixel_format_internal == 3){
		printf("internal pixel format: greyscale + alpha\n");
	}
	else if(pixel_format_internal == 4){
		printf("internal pixel format: rgb + alpha\n");
	}
	else if(pixel_format_internal == 127){
		printf("internal pixel format: indexed\n");
		int index_size = read_varint(in_bytes, &byte_pointer);
	}
	else if(pixel_format_internal == 128){
		printf("internal pixel format: subgreen\n");
	}
	else if(pixel_format_internal == 129){
		printf("internal pixel format: YIQ\n");
	}
	else if(pixel_format_internal == 130){
		printf("internal pixel format: subgreen + alpha\n");
	}
	else if(pixel_format_internal == 131){
		printf("internal pixel format: YIQ + alpha\n");
	}
	else if(pixel_format_internal == 255){
		printf("invalid internal pixel format!\n");
	}
	else{
		printf("unknown pixel format!\n");
	}
	return decoded;
}

int main(int argc, char *argv[]){
	if(argc == 2 && (strcmp(argv[1],"--help") == 0 || strcmp(argv[1],"-h") == 0)){
		print_usage();
		return 0;//technically everything went OK
	}
	else if(argc == 2 && strcmp(argv[1],"--version") == 0){
		printf("experimental software\n");
		return 0;
	}
	else if(argc < 3){
		printf("not enough arguments!\n");
		print_usage();
		return 1;
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);

	if(in_size < 8){
		printf("not a valid hoh file!\n");
		return 3;
	}
	int byte_pointer = 0;
	if(
		in_bytes[byte_pointer++] != 153
		|| in_bytes[byte_pointer++] != 72
		|| in_bytes[byte_pointer++] != 79
		|| in_bytes[byte_pointer++] != 72
	){
		printf("not a valid hoh file!\n");
		return 4;
	}
		printf("byte %d\n",(int)in_bytes[byte_pointer]);
		printf("byte %d\n",(int)in_bytes[byte_pointer + 1]);

		printf("byte %d\n",(int)in_bytes[byte_pointer + 2]);
		printf("byte %d\n",(int)in_bytes[byte_pointer + 3]);

		printf("byte %d\n",(int)in_bytes[byte_pointer + 4]);
		printf("byte %d\n",(int)in_bytes[byte_pointer + 5]);

	uint8_t pixel_format = in_bytes[byte_pointer++];
	if(pixel_format == 0){
		printf("pixel format: bit\n");
	}
	else if(pixel_format == 1){
		printf("pixel format: greyscale\n");
	}
	else if(pixel_format == 2){
		printf("pixel format: rgb\n");
	}
	else if(pixel_format == 3){
		printf("pixel format: greyscale + alpha\n");
	}
	else if(pixel_format == 4){
		printf("pixel format: rgb + alpha\n");
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
	int width = read_varint(in_bytes, &byte_pointer) + 1;
	int height = read_varint(in_bytes, &byte_pointer) + 1;
	printf("dimensions: %dx%d\n",width,height);
	if(bit_depth == 0){
		if(byte_pointer < in_size){
			printf("file is too large for a blank image!\n");
			return 4;
		}
		else{
			//write blank image
			printf("Parsing ended. Writing blank image not implemented!\n");
			return 5;
		}
	}
	uint8_t* decoded = decode_tile(
		in_bytes,
		in_size,
		byte_pointer,
		pixel_format,
		bit_depth,
		width,
		height
	);

	delete[] in_bytes;

	printf("Parsing ended. Decoding not completely implemented!\n");
	return 5;//not implemented status
}
