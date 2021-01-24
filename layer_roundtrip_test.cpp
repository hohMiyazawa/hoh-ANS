#include <stdio.h>

#include "layer_encode.hpp"
#include "layer_decode.hpp"

int main(int argc, char *argv[]){
	uint8_t test_image[20] = {
  1,  0,  1,  1,  5,
  1,255,  1,  1,  1,
254,  1,  1,  1,  1,
  1,  1, 14,  1,  1,
	};//5x4, intentionally not symmetric


//ENCODE
	printf("encoding...\n");

	uint16_t* buffer = new uint16_t[20];
	for(int i=0;i<20;i++){
		buffer[i] = (uint16_t)test_image[i];
	}

	uint8_t* LEMPEL_NUKE = new uint8_t[20];
	for(int i=0;i<20;i++){
		LEMPEL_NUKE[i] = 0;//no LZ freebies
	}

	uint8_t* compressed = new uint8_t[256];//give good room

	size_t data_size = layer_encode(
		buffer,
		20,
		5,//width
		4,//height
		8,//depth
		2,//enable some crunching features
		LEMPEL_NUKE,
		compressed
	);
	delete[] buffer;
	delete[] LEMPEL_NUKE;

//DECODE
	printf("decoding...\n");

	uint16_t* LEMPEL_BACKREF = new uint16_t[20];
	for(int i=0;i<20;i++){
		LEMPEL_BACKREF[i] = 0;//no LZ freebies
	}

	uint8_t* decompressed = decode_layer(
		compressed,
		data_size,
		0,
		5,//width
		4,//height
		8,//depth
		LEMPEL_BACKREF
	);
	delete[] LEMPEL_BACKREF;
	delete[] compressed;

	for(int i=0;i<20;i++){
		if(test_image[i] != decompressed[i]){
			printf("\033[0;31mLayer roundtrip: FAILED\033[0m\n");
			for(int i=0;i<20;i+=5){
				printf("%d %d %d %d %d\n",(int)decompressed[i],(int)decompressed[i+1],(int)decompressed[i+2],(int)decompressed[i+3],(int)decompressed[i+4]);
			}
			delete[] decompressed;
			return 1;
		}
	}
	printf("\033[0;32mLayer roundtrip: OK\033[0m\n");
	delete[] decompressed;

	return 0;
}
