#ifndef LZ_HEADER
#define LZ_HEADER

#include "channel_encode.hpp"

/*
int find_lz(
	uint16_t* source,
	size_t size,
	uint8_t* lz_symbols,
	size_t* lz_symbol_size,
	uint8_t* nukemap,
	int distance
){
	int lz_index = 0;
	int since_last = 0;
	int limit_distance = (1<<distance);


	uint8_t* lz_future = new uint8_t[size/3];
	uint8_t* lz_length = new uint8_t[size/3];
	uint8_t* lz_backby = new uint8_t[size/3];
	size_t lz_future_size = 0;
	size_t lz_length_size = 0;
	size_t lz_backby_size = 0;

	for(int i=0;i<size;i++){
		int longest = 0;
		int best_back = -1;
		for(int back=1;back<=limit_distance && i - back >= 0;back++){
			if(i - back < 0){
				break;
			}
			int offset = 0;
			while(
				i + offset < size
				&& source[i + offset] == source[i - back + offset]
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
		if(longest < 4){
			since_last++;
			if(since_last == 255){
				since_last = 0;
				lz_symbols[lz_index++] = 255;
				lz_future[lz_future_size++] = 255;
			}
		}
		else{
			lz_symbols[lz_index++] = since_last;
			lz_future[lz_future_size++] = since_last;
			if(distance > 8){
				lz_symbols[lz_index++] = (best_back - 1) / 256;
				lz_backby[lz_backby_size++] = (best_back - 1) / 256;
			}
			lz_symbols[lz_index++] = (best_back - 1) % 256;
			lz_backby[lz_backby_size++] = (best_back - 1) % 256;
			lz_symbols[lz_index++] = (longest - 4);
			lz_length[lz_length_size++] = (longest - 4);
			since_last = 0;
			for(int offset = 0;offset < longest;offset++){
				nukemap[i + offset] = 1;
			}
			i += longest - 1;
		}
	}
	*lz_symbol_size = lz_index;

	uint32_t* dummy1;
	uint32_t* dummy2;
	uint32_t* dummy3;
	int lz_overhead1 = channel_encode(
		lz_future,
		lz_future_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead2 = channel_encode(
		lz_length,
		lz_length_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead3 = channel_encode(
		lz_backby,
		lz_backby_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead_collected = channel_encode(
		lz_symbols,
		*lz_symbol_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	delete[] lz_future;
	delete[] lz_length;
	delete[] lz_backby;
	int best_size = lz_overhead1 + lz_overhead2 + lz_overhead3;
	if(lz_overhead_collected < best_size){
		best_size = lz_overhead_collected;
	}
	if(*lz_symbol_size < best_size){
		best_size = *lz_symbol_size;
	}
	//printf("internal lempel %d|%d + %d|%d + %d|%d = %d\n",lz_overhead1,(int)lz_future_size,lz_overhead2,(int)lz_length_size,lz_overhead3,(int)lz_backby_size,lz_overhead1 + lz_overhead2 + lz_overhead3);
	printf("internal lempel %d\n",best_size);
	return 1 + best_size;
}*/

int find_lz_rgb(
	uint8_t* source,
	size_t size,
	uint8_t* lz_symbols,
	size_t* lz_symbol_size,
	uint8_t* nukemap,
	int distance,
	int break_even_bonus
){
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
			if(i - back < 0){
				break;
			}
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
		if(longest < 4 + break_even_bonus){
			since_last++;
			if(since_last == 255){
				since_last = 0;
				lz_symbols[lz_index++] = 255;
				lz_future[lz_future_size++] = 255;
			}
		}
		else{
			lz_symbols[lz_index++] = since_last;
			lz_future[lz_future_size++] = since_last;
			if(distance > 8){
				lz_symbols[lz_index++] = (best_back - 1) / 256;
				lz_backby2[lz_backby2_size++] = (best_back - 1) / 256;
			}
			lz_symbols[lz_index++] = (best_back - 1) % 256;
			lz_backby[lz_backby_size++] = (best_back - 1) % 256;
			lz_symbols[lz_index++] = (longest - 4);
			lz_length[lz_length_size++] = (longest - 4);
			since_last = 0;
			for(int offset = 0;offset < longest;offset++){
				nukemap[i/3 + offset] = 1;
			}
			i += (longest - 1)*3;
		}
	}
	*lz_symbol_size = lz_index;

	uint32_t* dummy1;
	uint32_t* dummy2;
	uint32_t* dummy3;
	int lz_overhead1 = channel_encode(
		lz_future,
		lz_future_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead2 = channel_encode(
		lz_length,
		lz_length_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead3 = channel_encode(
		lz_backby,
		lz_backby_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead4 = 0;
	if(distance > 8){
		lz_overhead4 = channel_encode(
			lz_backby2,
			lz_backby2_size,
			10,
			dummy1,
			dummy2,
			dummy3
		);
	}
	int lz_overhead_collected = channel_encode(
		lz_symbols,
		*lz_symbol_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	delete[] lz_future;
	delete[] lz_length;
	delete[] lz_backby;
	delete[] lz_backby2;
	int best_size = lz_overhead1 + lz_overhead2 + lz_overhead3 + lz_overhead4;
	if(lz_overhead_collected < best_size){
		best_size = lz_overhead_collected;
	}
	if(*lz_symbol_size < best_size){
		best_size = *lz_symbol_size;
	}
	//printf("internal lempel %d|%d + %d|%d + %d|%d = %d\n",lz_overhead1,(int)lz_future_size,lz_overhead2,(int)lz_length_size,lz_overhead3,(int)lz_backby_size,lz_overhead1 + lz_overhead2 + lz_overhead3);
	//printf("external lempel %d\n",best_size);
	return 1 + best_size;
}

int find_lz_rgb_old(
	uint8_t* source,
	size_t size,
	uint8_t* lz_symbols,
	size_t* lz_symbol_size,
	uint8_t* nukemap,
	int distance,
	int break_even_bonus
){
	int lz_index = 0;
	int since_last = 0;
	int limit_distance = (1<<distance);


	uint8_t* lz_future = new uint8_t[size/9];
	uint8_t* lz_length = new uint8_t[size/9];
	uint8_t* lz_backby = new uint8_t[size/9];
	size_t lz_future_size = 0;
	size_t lz_length_size = 0;
	size_t lz_backby_size = 0;

	int stride_sum = 0;

	for(int i=0;i<size;i+=3){
		int longest = 0;
		int best_back = -1;
		for(int back = 1;back<=limit_distance && i - back*3 >= 0;back++){
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
		if(longest < 4 + break_even_bonus){
			since_last++;
			if(since_last == 255){
				since_last = 0;
				lz_symbols[lz_index++] = 255;
				lz_future[lz_future_size++] = 255;
			}
		}
		else{
			lz_symbols[lz_index++] = since_last;
			lz_future[lz_future_size++] = since_last;
			if(distance > 8){
				lz_symbols[lz_index++] = (best_back - 1) / 256;
				lz_backby[lz_backby_size++] = (best_back - 1) / 256;
			}
			lz_symbols[lz_index++] = (best_back - 1) % 256;
			lz_backby[lz_backby_size++] = (best_back - 1) % 256;
			lz_symbols[lz_index++] = (longest - 4);
			lz_length[lz_length_size++] = (longest - 4);
			since_last = 0;
			for(int offset = 0;offset < longest;offset++){
				nukemap[i/3 + offset] = 1;
			}
			i += (longest - 1)*3;
		}
	}
	*lz_symbol_size = lz_index;

	uint32_t* dummy1;
	uint32_t* dummy2;
	uint32_t* dummy3;
	int lz_overhead1 = channel_encode(
		lz_future,
		lz_future_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead2 = channel_encode(
		lz_length,
		lz_length_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead3 = channel_encode(
		lz_backby,
		lz_backby_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	int lz_overhead_collected = channel_encode(
		lz_symbols,
		*lz_symbol_size,
		10,
		dummy1,
		dummy2,
		dummy3
	);
	delete[] lz_future;
	delete[] lz_length;
	delete[] lz_backby;
	int best_size = lz_overhead1 + lz_overhead2 + lz_overhead3;
	if(lz_overhead_collected < best_size){
		best_size = lz_overhead_collected;
	}
	if(*lz_symbol_size < best_size){
		best_size = *lz_symbol_size;
	}
	//printf("internal lempel %d|%d + %d|%d + %d|%d = %d\n",lz_overhead1,(int)lz_future_size,lz_overhead2,(int)lz_length_size,lz_overhead3,(int)lz_backby_size,lz_overhead1 + lz_overhead2 + lz_overhead3);
	//printf("external lempel %d\n",best_size);
	//printf("stride sum %d\n",stride_sum);

	return 1 + best_size;
}
/*
void find_lz(
	uint16_t* source,
	size_t size,
	uint8_t* lz_symbols,
	size_t* lz_symbol_size,
	uint8_t* nukemap
){
	int lz_index = 0;
	int since_last = 0;
	for(int i=0;i<size;i++){
		int longest = 0;
		int best_back = -1;
		for(int back=1;back<=256;back++){
			if(i - back < 0){
				break;
			}
			int offset = 0;
			while(
				i + offset < size
				&& source[i + offset] == source[i - back + offset]
				&& offset < 260
			){
				offset++;
			}
			if(offset > longest){
				longest = offset;
				best_back = back;
			}
		}
		if(longest < 4){
			since_last++;
			if(since_last == 255){
				since_last = 0;
				lz_symbols[lz_index++] = 255;
			}
		}
		else{
			lz_symbols[lz_index++] = since_last;
			lz_symbols[lz_index++] = best_back - 1;
			lz_symbols[lz_index++] = longest - 4;
			since_last = 0;
			for(int offset = 0;offset < longest;offset++){
				nukemap[i + offset] = 1;
			}
			i += longest - 1;
		}
	}
	*lz_symbol_size = lz_index;
}
*/
/*
void find_lz(
	uint16_t* source,
	size_t size,
	uint16_t* lz_symbols,
	size_t* lz_symbol_size,
	uint8_t* nukemap
){
	int lz_index = 0;
	int since_last = 0;
	int range = 10;
	for(int i=0;i<size;i++){
		int longest = 0;
		int best_back = -1;
		for(int back=1;back<=(1<<range);back++){
			if(i - back < 0){
				break;
			}
			int offset = 0;
			while(
				i + offset < size
				&& source[i + offset] == source[i - back + offset]
				&& offset < ((1<<range)+4)
			){
				offset++;
			}
			if(offset > longest){
				longest = offset;
				best_back = back;
			}
		}
		if(longest < 4){
			since_last++;
			if(since_last == (1<<range) - 1){
				since_last = 0;
				lz_symbols[lz_index++] = (1<<range) - 1;
			}
		}
		else{
			lz_symbols[lz_index++] = since_last;
			lz_symbols[lz_index++] = best_back - 1;
			lz_symbols[lz_index++] = longest - 4;
			since_last = 0;
			for(int offset = 0;offset < longest;offset++){
				nukemap[i + offset] = 1;
			}
			i += longest - 1;
		}
	}
	*lz_symbol_size = lz_index;
}
*/
#endif // LZ_HEADER
