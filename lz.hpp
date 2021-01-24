#ifndef LZ_HEADER
#define LZ_HEADER

#include "entropy_encoding.hpp"

size_t find_lz_rgb(
	uint8_t* source,
	size_t size,
	int width,
	int height,
	uint8_t* lz_symbols,
	uint8_t* nukemap,
	int distance,
	int break_even_bonus
){
	size_t byte_pointer = 0;

	int lz_index = 0;
	int since_last = 0;
	int limit_distance = (1<<distance);


	uint8_t* lz_future = new uint8_t[size/9];
	uint8_t* lz_length = new uint8_t[size/9];
	uint8_t* lz_backby = new uint8_t[size/9];
	uint8_t* lz_backby2 = new uint8_t[size/9];
	size_t lz_future_size = 0;
	size_t lz_length_size = 0;
	size_t lz_backby_size = 0;
	size_t lz_backby2_size = 0;

	for(int i=0;i<size;i+=3){
		int longest = 0;
		int best_back = -1;
		for(int back=1;back<=limit_distance && i - back*3 >= 0;back++){
			int offset = 0;
			while(
				i + offset*3 < size
				&& source[i + offset*3] == source[i - back*3 + offset*3]
				&& source[i + offset*3 + 1] == source[i - back*3 + offset*3 + 1]
				&& source[i + offset*3 + 2] == source[i - back*3 + offset*3 + 2]
				&& offset < 259
			){
				offset++;
			}
			if(offset > longest){
				longest = offset;
				best_back = back;
				if(offset == 259){//short circuit
					back = limit_distance;
				}
			}
		}
		if(longest < 259 && distance > 8){
			for(int back=width;back <= (1<<16) && i - back*3 >= 0;back += width){
				int offset = 0;
				while(
					i + offset*3 < size
					&& source[i + offset*3] == source[i - back*3 + offset*3]
					&& source[i + offset*3 + 1] == source[i - back*3 + offset*3 + 1]
					&& source[i + offset*3 + 2] == source[i - back*3 + offset*3 + 2]
					&& offset < 259
				){
					offset++;
				}
				if(offset > longest){
					longest = offset;
					best_back = back;
					if(offset == 259){//short circuit
						back = limit_distance;
					}
				}
			}
		}
		if(longest < 4 + break_even_bonus){
			since_last++;
			if(since_last == 255){
				since_last = 0;
				//lz_symbols[lz_index++] = 255;
				lz_future[lz_future_size++] = 255;
			}
		}
		else{
			//lz_symbols[lz_index++] = since_last;
			lz_future[lz_future_size++] = since_last;
			if(distance > 8){
				//lz_symbols[lz_index++] = (best_back - 1) / 256;
				lz_backby2[lz_backby2_size++] = (best_back - 1) / 256;
			}
			//lz_symbols[lz_index++] = (best_back - 1) % 256;
			lz_backby[lz_backby_size++] = (best_back - 1) % 256;
			//lz_symbols[lz_index++] = (longest - 4);
			lz_length[lz_length_size++] = (longest - 4);
			since_last = 0;
			for(int offset = 0;offset < longest;offset++){
				nukemap[i/3 + offset] = 1;
			}
			i += (longest - 1)*3;
		}
	}
	//*lz_symbol_size = lz_index;

	lz_symbols[byte_pointer++] = 0b00000011;

	size_t lz_overhead1 = encode_entropy(
		lz_future,
		lz_future_size,
		256,
		lz_symbols + byte_pointer,
		10,
		1
	);
	delete[] lz_future;
	byte_pointer += lz_overhead1;
	size_t lz_overhead2 = encode_entropy(
		lz_length,
		lz_length_size,
		256,
		lz_symbols + byte_pointer,
		10,
		1
	);
	delete[] lz_length;
	byte_pointer += lz_overhead2;
	size_t lz_overhead3 = encode_entropy(
		lz_backby,
		lz_backby_size,
		256,
		lz_symbols + byte_pointer,
		10,
		1
	);
	delete[] lz_backby;
	byte_pointer += lz_overhead3;
	size_t lz_overhead4 = 0;

	if(distance > 8){
		lz_overhead4 = encode_entropy(
			lz_backby2,
			lz_backby2_size,
			256,
			lz_symbols + byte_pointer,
			10,
			1
		);
		byte_pointer += lz_overhead4;
	}
	delete[] lz_backby2;
	printf("lz groups %d %d %d %d\n",(int)lz_future_size,(int)lz_length_size,(int)lz_backby_size,(int)lz_backby2_size);
	//don't consider joined channels yet
	/*size_t lz_overhead_collected = encode_entropy(
		lz_symbols,
		*lz_symbol_size,
		256,
		dummyrand,
		10,
		0//no diagnostic
	);*/
	/*if(lz_overhead_collected < best_size){
		best_size = lz_overhead_collected;
	}*/
	/*if(*lz_symbol_size < best_size){
		best_size = *lz_symbol_size;
	}*/
	printf("internal lempel %d|%d + %d|%d + %d|%d + %d|%d = %d\n",
		(int)lz_overhead1,(int)lz_future_size,
		(int)lz_overhead2,(int)lz_length_size,
		(int)lz_overhead3,(int)lz_backby_size,
		(int)lz_overhead4,(int)lz_backby2_size,
		(int)(lz_overhead1 + lz_overhead2 + lz_overhead3 + lz_overhead4)
	);
	//printf("external lempel %d\n",best_size);
	printf("lz_total = %d\n",(int)byte_pointer);
	return byte_pointer;
}

#endif // LZ_HEADER
