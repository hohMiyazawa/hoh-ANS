#ifndef CHANNEL_HEADER
#define CHANNEL_HEADER

#include <assert.h>

static uint8_t* channel_picker8(uint8_t* source, size_t size, int total_channels, int target){
	assert(total_channels >= target + 1);
	assert(size % total_channels == 0);
	if(total_channels == 1){
		return source;
	}
	else{
		uint8_t* buf = new uint8_t[size/total_channels];
		for (size_t i=0; i < size/total_channels; i++) {
			buf[i] = source[i*total_channels + target];
		}
		return buf;
	}
}

int grey_test(uint8_t* source, size_t size){
	for (size_t i=0; i < size; i += 3) {
		if(
			source[i] != source[i + 1]
			|| source[i] != source[i + 2]
		){
			return 0;
		}
	}
	return 1;
}

int binary_test(uint8_t* source, size_t size){
	int col1 = source[0];
	int col2 = source[0];
	for (size_t i=0; i < size; i++) {
		if(
			source[i] != col1
		){
			if(col1 == col2){
				col2 = source[i];
			}
			else if(source[i] != col2){
				return 0;
			}
		}
	}
	return 1;
}

void binarize(uint8_t* source, size_t size){
	uint8_t col1 = source[0];
	for (size_t i=0; i < size; i++) {
		if(source[i] == col1){
			source[i] = 0;
		}
		else{
			source[i] = 1;
		}
	}
}

static uint16_t* channel_picker(uint8_t* source, size_t size, int total_channels, int target){
	assert(total_channels >= target + 1);
	assert(size % total_channels == 0);
	uint16_t* buf = new uint16_t[size/total_channels];
	for (size_t i=0; i < size/total_channels; i++) {
		buf[i] = source[i*total_channels + target];
	}
	return buf;
}

static void subtract_green(uint8_t* source, size_t size, uint16_t* GREEN, uint16_t* RED_G, uint16_t* BLUE_G){
	for (size_t i=0; i < size/3; i++) {
		GREEN[i]  = (uint16_t)source[i*3 + 1];
		RED_G[i]  = (uint16_t)((int)source[i*3 + 0] - (int)source[i*3 + 1] + 256);
		BLUE_G[i] = (uint16_t)((int)source[i*3 + 2] - (int)source[i*3 + 1] + 256);
	}
}

static void rgb_to_yiq(uint8_t* source, size_t size, uint8_t* Y, uint16_t* I, uint16_t* Q){
	for (size_t i=0; i < size/3; i++) {
		uint8_t R = source[i*3 + 0];
		uint8_t G = source[i*3 + 1];
		uint8_t B = source[i*3 + 2];
		Y[i] = (((R + B)>>1) + G)>>1;
		I[i] = (uint16_t)((int)R - (int)B + 256);
		Q[i] = (uint16_t)((int)G - (((int)R + (int)B)>>1) + 256);
	}
}

static void palette_compact(uint8_t* source, size_t size){
	int histogram[256];
	for(int i=0;i<256;i++){
		histogram[i] = 0;
	}
	for (size_t i=0; i < size; i++) {
		histogram[source[i]]++;
	}
	int moving_table[256];
	int diff = 0;
	for(int i=0;i<256;i++){
		moving_table[i] = diff;
		if(histogram[i] == 0){
			diff++;
		}
	}
	for (size_t i=0; i < size; i++) {
		source[i] = source[i] - moving_table[source[i]];
	}
}

#endif // CHANNEL_HEADER
