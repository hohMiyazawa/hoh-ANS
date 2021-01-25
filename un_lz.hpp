#ifndef UN_LZ_HEADER
#define UN_LZ_HEADER

#include "entropy_decoding.hpp"

uint16_t* un_lz(
	uint8_t* in_bytes,
	size_t in_size,
	size_t* byte_pointer,
	size_t width,
	size_t height
){
	uint8_t lz_type = in_bytes[(*byte_pointer)++];
	uint8_t use_lempel   = (lz_type & 0b00000001);
	uint8_t backref_size = (lz_type & 0b00000010)>>1;
	uint8_t joined       = (lz_type & 0b00000100)>>2;
	uint16_t* LEMPEL_BACKREF = new uint16_t[width*height];
	if(use_lempel == 0){
		for(size_t i=0;i<width*height;i++){
			LEMPEL_BACKREF[i] = 0;
		}
		printf("no un_lz to do\n");
		return LEMPEL_BACKREF;
	}
	if(backref_size == 0){//1byte backrefs, not implemented yet
		printf("unimplemented un_lz\n");
		if(joined){
		}
		else{
		}
	}
	else{
		printf("standard un_lz\n");
		if(joined){//not implemented
		}
		else{
			printf("pointer %d \n",(int)(*byte_pointer));
			printf("in_size %d \n",(int)(in_size));

			size_t lz_future_size;
			uint8_t* lz_future = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&lz_future_size,
				1
			);
			printf("%d \n",(int)lz_future_size);

			printf("pointer %d \n",(int)(*byte_pointer));

			size_t lz_length_size;
			uint8_t* lz_length = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&lz_length_size,
				1
			);
			printf("%d \n",(int)lz_length_size);

			printf("pointer %d \n",(int)(*byte_pointer));

			size_t lz_backby_size;
			uint8_t* lz_backby = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&lz_backby_size,
				1
			);
			printf("%d \n",(int)lz_backby_size);

			printf("pointer %d \n",(int)(*byte_pointer));

			size_t lz_backby2_size;
			uint8_t* lz_backby2 = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&lz_backby2_size,
				1
			);
			printf("%d\n",(int)lz_backby2_size);

			printf("pointer %d \n",(int)(*byte_pointer));

			size_t index = 0;

			size_t group = 0;

			for(size_t i=0;i<lz_future_size;i++){
				size_t count = 0;
				while(lz_future[i] == 255){
					count += 255;
					i++;
				};
				count += lz_future[i];
				for(size_t j=0;j<count;j++){
					LEMPEL_BACKREF[index++] = 0;
				}
				size_t length = (size_t)(lz_length[group]) + 4;
				uint16_t backby = (((uint16_t)(lz_backby2[group]))<<8) + (uint16_t)(lz_backby[group]);
				group++;
				for(size_t j=0;j<length;j++){
					LEMPEL_BACKREF[index++] = backby;
				}
			}
			printf("GROUP %d\n",(int)group);

			delete[] lz_future;
			delete[] lz_length;
			delete[] lz_backby;
			delete[] lz_backby2;
		}
	}
	return LEMPEL_BACKREF;
}

#endif // UN_LZ_HEADER
