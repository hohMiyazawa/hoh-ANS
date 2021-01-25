#include "platform.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "varint.hpp"
#include "file_io.hpp"
#include "bitimage.hpp"
#include "entropy_decoding.hpp"
#include "layer_decode.hpp"
#include "un_lz.hpp"

void print_usage(){
	printf("usage: dhoh infile.hoh outfile.rgb\n");
	printf("The output file consist of raw 8bit RGB bytes\n");
	printf("You can turn that into an image with imagemagick:\n");
	printf("[TODO: imagemagick command]\n");
}

uint8_t* decode_tile(
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

	uint8_t* decoded = new uint8_t[width*height*channel_number];

	if(x_tiles != 1 || y_tiles != 1){
		printf("tiled image: %dx%d (%d tiles)\n",(int)x_tiles,(int)y_tiles,(int)x_tiles * (int)y_tiles);
		size_t offsets[x_tiles*y_tiles];
		offsets[0] = 0;
		printf("  tile offset 0: 0\n");
		for(size_t i=1;i<x_tiles*y_tiles;i++){
			offsets[i] = read_varint(in_bytes, &byte_pointer);
			printf("  tile offset %d: %d\n",(int)i,(int)offsets[i]);
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
			uint8_t* decoded_tile_data = decode_tile(
				in_bytes,
				in_size,
				byte_pointer + cummulative_offset,
				pixel_format,
				channel_number,
				bit_depth,
				new_width,
				new_height
			);
			for(size_t y=0;y<new_height;y++){
				for(size_t x=0;x<new_width;x++){
					for(size_t chan=0;chan<channel_number;chan++){
						decoded[
							((y + y_offset) * width + x + x_offset)*channel_number
							+ chan
						] = decoded_tile_data[
							(y * new_width + x)*channel_number
							+ chan
						];
					}
				}
			}
			delete[] decoded_tile_data;
		}
		return decoded;
	}
	else{
		printf("single tile %dx%d\n",(int)width,(int)height);
	}
	if(bit_depth > 8){
		printf("high bit depth not implemented!\n");
	}
	uint8_t pixel_format_internal = in_bytes[byte_pointer++];
	uint8_t channel_number_internal = 1;
	if(pixel_format == 0){
		printf("  internal pixel format: bit\n");
	}
	else if(pixel_format_internal == 1){
		printf("  internal pixel format: greyscale\n");
	}
	else if(pixel_format_internal == 2){
		printf("  internal pixel format: rgb\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 3){
		printf("  internal pixel format: greyscale + alpha\n");
		channel_number_internal = 2;
	}
	else if(pixel_format_internal == 4){
		printf("  internal pixel format: rgb + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 127){
		printf("  internal pixel format: indexed\n");
		int index_size = read_varint(in_bytes, &byte_pointer);
		printf("  index length: %d\n",index_size);
	}
	else if(pixel_format_internal == 128){
		printf("  internal pixel format: subgreen\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 129){
		printf("  internal pixel format: YIQ\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 130){
		printf("  internal pixel format: subgreen + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 131){
		printf("  internal pixel format: YIQ + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 255){
		printf("  invalid internal pixel format!\n");
	}
	else{
		printf("  unknown pixel format %d!\n",(int)pixel_format_internal);
	}
	
	uint16_t* LEMPEL_BACKREF = un_lz(
		in_bytes,
		in_size,
		&byte_pointer,
		width,
		height
	);

	printf("WHAT WHAT WHAT\n");//DEBUG

	if(channel_number_internal == 1){
		//no need for additional channel information
		uint8_t* decoded_channel = decode_layer(
			in_bytes,
			in_size,
			byte_pointer,
			width,
			height,
			bit_depth,
			LEMPEL_BACKREF
		);
		if(pixel_format == pixel_format_internal){
			delete[] decoded;
			return decoded_channel;
		}
	}
	else if(channel_number_internal == 2){
		uint8_t channel_reordering = in_bytes[byte_pointer++];
		uint8_t bit1 = channel_reordering & 1;
		uint8_t bit2 = channel_reordering & 2;
		size_t offset = 0;
		if(bit1 == 0 && bit2 == 0){
			//conjoined
			printf("  conjoined channels\n");
		}
		else if(bit1 == 0 && bit2 == 1){
			//regular order
			offset = read_varint(in_bytes, &byte_pointer);
			printf("  regular channel layout\n");
		}
		else if(bit1 == 1 && bit2 == 0){
			//inverse order
			offset = read_varint(in_bytes, &byte_pointer);
			printf("  inverse channel layout\n");
		}
		else{
			//illegal
			printf("  illegal channel layout!\n");
		}

		uint8_t* decoded_channel1 = decode_layer(
			in_bytes,
			in_size,
			byte_pointer,
			width,
			height,
			bit_depth,
			LEMPEL_BACKREF
		);
		uint8_t* decoded_channel2 = decode_layer(
			in_bytes,
			in_size,
			byte_pointer + offset,
			width,
			height,
			bit_depth,
			LEMPEL_BACKREF
		);
		for(size_t i=0;i<width*height;i++){
			decoded[i*3]     = decoded_channel2[i];
			decoded[i*3 + 1] = decoded_channel1[i];
		}
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
			printf("  conjoined channels\n");
			offset1 = 0;
			offset2 = 0;
		}
		else if(bit1 != bit2 && bit1 != bit3){
			offset1 = read_varint(in_bytes, &byte_pointer);
			offset2 = read_varint(in_bytes, &byte_pointer);
			printf("  channel order: %d %d %d\n",(int)bit1,(int)bit2,(int)bit3);
		}
		else{
			offset1 = read_varint(in_bytes, &byte_pointer);
			offset2 = 0;
			printf("  semi-conjoined channels\n");
		}

		//conjoined channels not decoded
		uint8_t* decoded_channel1 = decode_layer(
			in_bytes,
			in_size,
			byte_pointer,
			width,
			height,
			bit_depth,
			LEMPEL_BACKREF
		);
		uint8_t* decoded_channel2 = decode_layer(
			in_bytes,
			in_size,
			byte_pointer + offset1,
			width,
			height,
			bit_depth,
			LEMPEL_BACKREF
		);
		uint8_t* decoded_channel3 = decode_layer(
			in_bytes,
			in_size,
			byte_pointer + offset1 + offset2,
			width,
			height,
			bit_depth,
			LEMPEL_BACKREF
		);
		if(pixel_format == 2){
			//default order GRB, not honoring channel reordering at the moment
			for(size_t i=0;i<width*height;i++){
				decoded[i*3]     = decoded_channel2[i];//red
				decoded[i*3 + 1] = decoded_channel1[i];//green
				decoded[i*3 + 2] = decoded_channel3[i];//blue
				//printf("pixel %d %03d,%03d,%03d\n",i,(int)decoded[i*3],(int)decoded[i*3+1],(int)decoded[i*3+2]);
			}
		}
	}
	else if(channel_number_internal == 4){
	}

	delete[] LEMPEL_BACKREF;
	return decoded;
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
		channel_number,
		bit_depth,
		width,
		height
	);
	printf("writing output to \"%s\"\n",argv[2]);
	write_file(argv[2], decoded, width*height*channel_number);

	delete[] decoded;
	delete[] in_bytes;

	return 0;
}
