#ifndef LZ_HEADER
#define LZ_HEADER

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

#endif // LZ_HEADER
