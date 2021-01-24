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
		return LEMPEL_BACKREF;
	}
	if(backref_size == 0){//1byte backrefs, not implemented yet
		if(joined){
		}
		else{
		}
	}
	else{
		if(joined){//not implemented
		}
		else{
			size_t symbol1_size;
			uint8_t* entropy1 = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&symbol1_size,
				1
			);
			size_t symbol2_size;
			uint8_t* entropy2 = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&symbol2_size,
				1
			);
			size_t symbol3_size;
			uint8_t* entropy3 = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&symbol3_size,
				1
			);
			size_t symbol4_size;
			uint8_t* entropy4 = decode_entropy_8bit(
				in_bytes,
				in_size,
				byte_pointer,
				&symbol4_size,
				1
			);
		}
	}
	return LEMPEL_BACKREF;
}

#endif // UN_LZ_HEADER
