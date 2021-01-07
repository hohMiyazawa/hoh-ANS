#ifndef CHANNEL_HEADER
#define CHANNEL_HEADER

#include <assert.h>

static uint8_t* channel_picker(uint8_t* source, size_t size, int total_channels, int target, size_t* out_size){
	assert(total_channels >= target + 1);
	assert(size % total_channels == 0);
	if(total_channels == 1){
		*out_size = size;
		return source;
	}
	else{
		*out_size = size/total_channels;
		uint8_t* buf = new uint8_t[size/total_channels];
		for (size_t i=0; i < *out_size; i++) {
			buf[i] = source[i*total_channels + target];
		}
		return buf;//wrong, a placeholder
	}
}

static void subtract_green(uint8_t* source, size_t size, uint8_t* GREEN, uint16_t* RED_G, uint16_t* BLUE_G){
	for (size_t i=0; i < size/3; i++) {
		GREEN[i]  = source[i*3 + 1];
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

#endif // CHANNEL_HEADER
