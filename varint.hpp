#ifndef VARINT_HEADER
#define VARINT_HEADER

#include <string.h>

size_t read_varint(uint8_t* bytes, size_t* location){//incorrect implentation! only works up to three bytes/21bit numbers!
	uint8_t first_byte = bytes[*location];
	*location = *location + 1;
	if(first_byte & (1<<7)){
		uint8_t second_byte = bytes[*location];
		*location = *location + 1;
		if(second_byte & (1<<7)){
			uint8_t third_byte = bytes[*location];
			*location = *location + 1;
			//printf("dvar %d %d %d\n",(int)first_byte,(int)second_byte,(int)third_byte);
			return (((size_t)first_byte & 0b01111111) << 14) + (((size_t)second_byte & 0b01111111) << 7) + third_byte;
		}
		else{
			//printf("dvar %d %d\n",(int)first_byte,(int)second_byte);
			return (((size_t)first_byte & 0b01111111) << 7) + second_byte;
		}
	}
	else{
		//printf("dvar %d\n",(int)first_byte);
		return (size_t)first_byte;
	}
}

void write_varint(uint8_t* bytes, size_t* location, size_t value){//must be improved
	if(value < (1<<7)){
		bytes[(*location)++] = (uint8_t)value;
		//printf("varint %d, %d\n",value,value);
	}
	else if(value < (1<<14)){
		bytes[(*location)++] = (uint8_t)((value>>7) + 128);
		bytes[(*location)++] = (uint8_t)(value % 128);
		//printf("varint %d, %d %d\n",value,(uint8_t)((value>>7) + 128),(uint8_t)(value % 128));
	}
	else if(value < (1<<21)){
		bytes[(*location)++] = (uint8_t)((value>>14) + 128);
		bytes[(*location)++] = (uint8_t)(((value>>7) % 128) + 128);
		bytes[(*location)++] = (uint8_t)(value % 128);
		//printf("varint %d, %d %d %d\n",value,(uint8_t)((value>>14) + 128),(uint8_t)(((value>>7) % 128) + 128),(uint8_t)(value % 128));
	}
}

void stuffer(
	uint8_t* bytes,
	size_t* location,
	uint8_t* remainder,
	uint8_t* bits_remaining,
	uint32_t value,
	uint8_t bits
){
	if(bits < *bits_remaining){
		*remainder = *remainder + (uint8_t)(value<<(*bits_remaining - bits));
		*bits_remaining = *bits_remaining - bits;
	}
	else if(bits == *bits_remaining){
		bytes[(*location)++] = *remainder + (uint8_t)value;
		*remainder = 0;
		*bits_remaining = 8;
	}
	else if(bits > *bits_remaining){
		if(bits > 8){
			uint32_t top = value>>8;
			uint32_t bottom = value % 256;
			stuffer(bytes, location, remainder, bits_remaining, top,    bits - 8);
			stuffer(bytes, location, remainder, bits_remaining, bottom, 8);
		}
		else{
			bytes[(*location)++] = *remainder + (uint8_t)(value>>(bits - *bits_remaining));
			*bits_remaining = (8 - (bits - *bits_remaining));
			*remainder = (uint8_t)((value<<(*bits_remaining)) % 256);
		}
	}
}

uint32_t unstuffer(
	uint8_t* bytes,
	size_t* location,
	uint8_t* remainder,
	uint8_t* bits_remaining,
	uint8_t bits
){
	uint32_t value = 0;
	if(bits <= *bits_remaining){
		*bits_remaining = *bits_remaining - bits;
		value = (*remainder)>>(*bits_remaining);
		*remainder = (*remainder) % (1 << (*bits_remaining));
	}
	else{
		bits = bits - *bits_remaining;
		value = (*remainder)<<bits;
		*remainder = bytes[(*location)++];
		*bits_remaining = 8;
		return value + unstuffer(
			bytes,
			location,
			remainder,
			bits_remaining,
			bits
		);
	}
	return value;
}

#endif //VARINT_HEADER
