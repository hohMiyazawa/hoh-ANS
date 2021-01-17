#ifndef VARINT_HEADER
#define VARINT_HEADER

size_t read_varint(uint8_t* bytes, size_t* location){//incorrect implentation! only works up to three bytes/21bit numbers!
	uint8_t first_byte = bytes[*location];
	*location = *location + 1;
	if(first_byte & (1<<7)){
		uint8_t second_byte = bytes[*location];
		*location = *location + 1;
		if(second_byte & (1<<7)){
			uint8_t third_byte = bytes[*location];
			*location = *location + 1;
			return (((size_t)first_byte & 0b01111111) << 14) + (((size_t)second_byte & 0b01111111) << 7) + third_byte;
		}
		else{
			return (((size_t)first_byte & 0b01111111) << 7) + second_byte;
		}
	}
	else{
		return (size_t)first_byte;
	}
}

void write_varint(uint8_t* bytes, size_t* location, size_t value){//must be improved
	if(value < 128){
		bytes[*location] = (uint8_t)value;
		*location = *location + 1;
	}
	else if(value < (1<<13)){
		bytes[*location] = (uint8_t)((value>>7) + 128);
		*location = *location + 1;
		bytes[*location] = (uint8_t)(value % 128);
		*location = *location + 1;
	}
}

#endif //VARINT_HEADER
