#ifndef LAYER_DECODE_HEADER
#define LAYER_DECODE_HEADER

#include "entropy_decoding.hpp"

uint8_t* decode_layer(
	uint8_t* in_bytes,
	size_t in_size,
	size_t byte_pointer,
	size_t width,
	size_t height,
	uint8_t bit_depth
){
	uint8_t channel_transforms = in_bytes[byte_pointer++];
	uint8_t compaction_mode = (channel_transforms & 0b11100000)>>5;
	uint8_t prediction_mode = (channel_transforms & 0b00010000)>>4;

	uint8_t* decoded = new uint8_t[width*height];
	uint8_t colour_map[1<<bit_depth];
	if(compaction_mode == 0){
		printf("    no channel compaction\n");
		//as-is
		for(int i=0;i<(1<<bit_depth);i++){
			colour_map[i] = i;
		}
	}
	else if(compaction_mode == 1){
		printf("    clamped channel\n");
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
		printf("    bitmasked channel\n");
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

		size_t x_tiles = in_bytes[byte_pointer++] + 1;
		size_t y_tiles =  + 1;
		if(x_tiles == 1 && y_tiles == 1){
			uint16_t predictor_bits = in_bytes[byte_pointer++];
			uint16_t lower_bits = in_bytes[byte_pointer++];
			predictor_bits = (predictor_bits<<8) + lower_bits;
		}
		else{
			uint8_t combinations_used = in_bytes[byte_pointer++];
			uint16_t combinations[combinations_used];
			for(int i=0;i<combinations_used;i++){
				uint16_t upper_bits = in_bytes[byte_pointer++];
				uint16_t lower_bits = in_bytes[byte_pointer++];
				combinations[i] = (upper_bits<<8) + lower_bits;
			}
		}
	}
	else{
		size_t symbol_size;
		uint16_t* symbols = decode_entropy(
			in_bytes,
			in_size,
			&byte_pointer,
			&symbol_size
		);
		for(int i=0;i<symbol_size;i++){
			decoded[i] = (uint8_t)symbols[i];
		}
	}
	return decoded;
}

#endif // LAYER_DECODE_HEADER
