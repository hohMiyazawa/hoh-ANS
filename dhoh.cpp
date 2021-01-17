#include "platform.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "rans64.hpp"
#include "file_io.hpp"
#include "channel.hpp"
#include "bitimage.hpp"

void print_usage(){
	printf("usage: dhoh infile.hoh outfile.rgb\n");
	printf("The output file consist of raw 8bit RGB bytes\n");
	printf("You can turn that into an image with imagemagick:\n");
	printf("[TODO: imagemagick command]\n");
}

int read_varint(uint8_t* bytes, int* location){//incorrect implentation! only works up to three bytes/21bit numbers!
	uint8_t first_byte = bytes[*location];
	*location = *location + 1;
	if(first_byte & (1<<7)){
		uint8_t second_byte = bytes[*location];
		*location = *location + 1;
		if(second_byte & (1<<7)){
			uint8_t third_byte = bytes[*location];
			*location = *location + 1;
			return (((int)first_byte & 0b01111111) << 14) + (((int)second_byte & 0b01111111) << 7) + third_byte;
		}
		else{
			return (((int)first_byte & 0b01111111) << 7) + second_byte;
		}
	}
	else{
		return (int)first_byte;
	}
}

uint16_t* decode_rans(
	uint8_t* in_bytes,
	size_t in_size,
	int byte_pointer,
	int symbol_size
){
	int symbol_range = read_varint(in_bytes, &byte_pointer);
	uint8_t metadata = in_bytes[byte_pointer++];
	uint8_t prob_bits = metadata>>4;
	uint8_t storage_mode = metadata % (1<<4);

	uint32_t freqs[symbol_range];
	uint32_t cum_freqs[symbol_range + 1];
	Rans64DecSymbol dsyms[symbol_range];

	if(storage_mode == 0){
		for(int i=0;i<symbol_range;i++){
			freqs[i] = 1;
		}
	}
	else if(storage_mode == 1){
	}
	else if(storage_mode == 2){
	}
	else if(storage_mode == 3){
	}
	else{
		printf("unknown frequency table storage mode!\n");
	}
	int data_size = read_varint(in_bytes, &byte_pointer);
	for(int i=0; i < symbol_range; i++) {
		Rans64DecSymbolInit(&dsyms[i], cum_freqs[i], freqs[i]);
	}
	uint16_t cum2sym[1<<prob_bits];
	for(int s=0; s < symbol_range; s++){
		for(uint32_t i=cum_freqs[s]; i < cum_freqs[s+1]; i++){
	   		 cum2sym[i] = s;
		}
	}

        Rans64State rans;
        uint32_t* ptr = (uint32_t*)(in_bytes + byte_pointer);
        Rans64DecInit(&rans, &ptr);
	
	uint16_t* decoded = new uint16_t[symbol_size];

	for(size_t i=0; i < symbol_size; i++) {
		uint32_t s = cum2sym[Rans64DecGet(&rans, prob_bits)];
		decoded[i] = (uint16_t) s;
		Rans64DecAdvanceSymbol(&rans, &ptr, &dsyms[s], prob_bits);
	}
	return decoded;
}

uint8_t* decode_channel(
	uint8_t* in_bytes,
	size_t in_size,
	int byte_pointer,
	uint8_t pixel_format,
	uint8_t bit_depth,
	int width,
	int height
){
	uint8_t channel_transforms = in_bytes[byte_pointer++];
	uint8_t compaction_mode = (channel_transforms & 0b11100000)>>5;
	uint8_t prediction_mode = (channel_transforms & 0b00010000)>>4;
	uint8_t entropy_mode    = (channel_transforms & 0b00001111);
	uint8_t* decoded = new uint8_t[width*height];
	uint8_t colour_map[1<<bit_depth];
	if(compaction_mode == 0){
		printf("no channel compaction\n");
		//as-is
		for(int i=0;i<(1<<bit_depth);i++){
			colour_map[i] = i;
		}
	}
	else if(compaction_mode == 1){
		printf("clamped channel\n");
		//clamping
		if(bit_depth == 8){
			uint8_t clamp1 = in_bytes[byte_pointer++];
			uint8_t clamp2 = in_bytes[byte_pointer++];
			if(clamp1 > clamp2){
				//unsure what this should mean
			}
			else{
			}
		}
		else{
			printf("unimplemented bit depth!\n");
		}
	}
	else if(compaction_mode == 2){
		printf("bitmasked channel\n");
		//bitmask
		uint8_t bitmask[(1<<bit_depth)/8];
		for(int i=0;i<(1<<bit_depth)/8;i++){
			bitmask[i] = in_bytes[byte_pointer++];
		}
	}
	else if(compaction_mode == 3){
		//laplace???
	}
	else if(compaction_mode == 4){
		uint8_t pointernumber = in_bytes[byte_pointer++];
		uint8_t pointers[pointernumber];
		for(int i=0;i<pointernumber;i++){
			pointers[i] = in_bytes[byte_pointer++];
		}
	}
	else if(compaction_mode == 5){
		uint8_t clamp1 = in_bytes[byte_pointer++];
		uint8_t clamp2 = in_bytes[byte_pointer++];
		uint8_t bitmask[(clamp2 - clamp1 + 8)/8];
		for(int i=0;i<(clamp2 - clamp1 + 8)/8;i++){
			bitmask[i] = in_bytes[byte_pointer++];
		}
	}
	else if(compaction_mode == 6){
	}
	else if(compaction_mode == 7){
	}
	if(prediction_mode){
		printf("unimplemented prediction mode!\n");
	}
	else{
		if(entropy_mode == 0){
			if(bit_depth == 8){
				for(int i=0;i<width*height;i++){
					decoded[i] = in_bytes[byte_pointer++];
				}
			}
			else{
				printf("unimplemented bit depth!\n");
			}
		}
		else{
			printf("unimplemented entropy mode!\n");
		}
	}
	return decoded;
}

uint8_t* decode_tile(
	uint8_t* in_bytes,
	size_t in_size,
	int byte_pointer,
	uint8_t pixel_format,
	uint8_t channel_number,
	uint8_t bit_depth,
	int width,
	int height
){
	uint8_t x_tiles = in_bytes[byte_pointer++] + 1;
	uint8_t y_tiles = in_bytes[byte_pointer++] + 1;

	uint8_t* decoded = new uint8_t[width*height*channel_number];

	if(x_tiles != 1 || y_tiles != 1){
		printf("tiled image: %dx%d\n",(int)x_tiles,(int)y_tiles);
		int offsets[x_tiles*y_tiles];
		offsets[0] = 0;
		printf("tile offset 0: 0\n");
		for(int i=1;i<x_tiles*y_tiles;i++){
			offsets[i] = read_varint(in_bytes, &byte_pointer);
			printf("tile offset %d: %d\n",i,offsets[i]);
		}

		int tile_width = (width + x_tiles - 1)/x_tiles;
		int tile_height = (height + y_tiles - 1)/y_tiles;
		int cummulative_offset = 0;
		for(int i=0;i<x_tiles*y_tiles;i++){
			cummulative_offset += offsets[i];
			int x_offset = (i % x_tiles) * tile_width;
			int y_offset = (i / x_tiles) * tile_height;
			int new_width = tile_width;
			int new_height = tile_height;
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
			for(int y=0;y<new_height;y++){
				for(int x=0;x<new_width;x++){
					decoded[(y + y_offset) * width + x + x_offset] = decoded_tile_data[y * new_width + x];
				}
			}
			delete[] decoded_tile_data;
		}
		return decoded;
	}
	else{
		printf("single tile image\n");
	}
	if(bit_depth > 8){
		printf("high bit depth not implemented!\n");
	}
	uint8_t pixel_format_internal = in_bytes[byte_pointer++];
	uint8_t channel_number_internal = 1;
	if(pixel_format == 0){
		printf("internal pixel format: bit\n");
	}
	else if(pixel_format_internal == 1){
		printf("internal pixel format: greyscale\n");
	}
	else if(pixel_format_internal == 2){
		printf("internal pixel format: rgb\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 3){
		printf("internal pixel format: greyscale + alpha\n");
		channel_number_internal = 2;
	}
	else if(pixel_format_internal == 4){
		printf("internal pixel format: rgb + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 127){
		printf("internal pixel format: indexed\n");
		int index_size = read_varint(in_bytes, &byte_pointer);
		printf("index length: %d\n",index_size);
	}
	else if(pixel_format_internal == 128){
		printf("internal pixel format: subgreen\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 129){
		printf("internal pixel format: YIQ\n");
		channel_number_internal = 3;
	}
	else if(pixel_format_internal == 130){
		printf("internal pixel format: subgreen + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 131){
		printf("internal pixel format: YIQ + alpha\n");
		channel_number_internal = 4;
	}
	else if(pixel_format_internal == 255){
		printf("invalid internal pixel format!\n");
	}
	else{
		printf("unknown pixel format!\n");
	}
	uint8_t lempel_ziv_mode = in_bytes[byte_pointer++];
	if(lempel_ziv_mode == 0){
		printf("no Lempel-Ziv transform\n");
	}

	if(channel_number_internal == 1){
		//no need for additional channel information
		uint8_t* decoded_channel = decode_channel(
			in_bytes,
			in_size,
			byte_pointer,
			pixel_format_internal,
			bit_depth,
			width,
			height
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
		if(bit1 == 0 && bit2 == 0){
			//conjoined
			printf("conjoined channels\n");
		}
		else if(bit1 == 0 && bit2 == 1){
			//regular order
			int offset = read_varint(in_bytes, &byte_pointer);
			printf("regular channel layout\n");
		}
		else if(bit1 == 1 && bit2 == 0){
			//inverse order
			int offset = read_varint(in_bytes, &byte_pointer);
			printf("inverse channel layout\n");
		}
		else{
			//illegal
			printf("illegal channel layout!\n");
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
			printf("conjoined channels\n");
		}
		else if(bit1 != bit2 && bit1 != bit3){
			offset1 = read_varint(in_bytes, &byte_pointer);
			offset2 = read_varint(in_bytes, &byte_pointer);
			printf("channel order: %d %d %d\n",(int)bit1,(int)bit2,(int)bit3);
		}
		else{
			offset1 = read_varint(in_bytes, &byte_pointer);
			printf("semi-conjoined channels\n");
		}

		//conjoined channels not decoded
		uint8_t* decoded_channel1 = decode_channel(
			in_bytes,
			in_size,
			byte_pointer,
			pixel_format_internal,
			bit_depth,
			width,
			height
		);
		uint8_t* decoded_channel2 = decode_channel(
			in_bytes,
			in_size,
			byte_pointer + offset1,
			pixel_format_internal,
			bit_depth,
			width,
			height
		);
		uint8_t* decoded_channel3 = decode_channel(
			in_bytes,
			in_size,
			byte_pointer + offset1 + offset2,
			pixel_format_internal,
			bit_depth,
			width,
			height
		);
		if(pixel_format == 2){
			//default order GRB, not honoring channel reordering at the moment
			for(int i=0;i<width*height;i++){
				decoded[i*3]     = decoded_channel2[i];//red
				decoded[i*3 + 1] = decoded_channel1[i];//green
				decoded[i*3 + 2] = decoded_channel3[i];//blue
				//printf("pixel %d %03d,%03d,%03d\n",i,(int)decoded[i*3],(int)decoded[i*3+1],(int)decoded[i*3+2]);
			}
		}
	}
	else if(channel_number_internal == 4){
	}
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
