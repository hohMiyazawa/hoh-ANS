#ifndef UNPREDICTION_HEADER
#define UNPREDICTION_HEADER

#include "predictor_operations.hpp"

uint16_t* unpredict_all(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	int x_tiles,
	int y_tiles,
	uint16_t* tile_map,
	uint16_t* LEMPEL_BACKREF
){
	int centre = 1<<depth;
	int tile_width  = (width  + x_tiles - 1)/x_tiles;
	int tile_height = (height + y_tiles - 1)/y_tiles;
	uint16_t* out_buf = new uint16_t[width*height];

	int best_pred[width];
	for(int i=0;i<width;i++){
		best_pred[i] = 4;
	}

	uint16_t top_row[width];
	uint16_t forige;
	uint16_t forige_TL;
	for(int i=0;i<width;i++){
		top_row[i] = centre/2;
	}
	size_t index = 0;

	for(int y_m=0;y_m < height;y_m++){
		forige    = centre/2;
		forige_TL = centre/2;
		for(int x_m=0;x_m < width;x_m++){
			int datalocation = y_m*width + x_m;
			uint16_t predictor = tile_map[(y_m/tile_height) * x_tiles + (x_m/tile_width)];
			uint16_t L = forige;
			uint16_t T = top_row[x_m];
			uint16_t TL = forige_TL;
			uint16_t TR = top_row[(x_m + width + 1) % width];
			uint16_t predictions[16] = {
				L,
				T,
				TL,
				TR,
				median(T,L,T+L-TL),
				midpoint(L,T),
				midpoint(L,TL),
				midpoint(TL,T),
				midpoint(T,TR),
				paeth(L,TL,T),
				average3(L,L,TL),
				average3(L,TL,TL),
				average3(TL,TL,T),
				average3(TL,T,T),
				average3(T,T,TR),
				average3(T,TR,TR)
			};
			if(LEMPEL_BACKREF[datalocation]){
				out_buf[datalocation] = out_buf[datalocation - LEMPEL_BACKREF[datalocation]];
			}
			else{
				uint16_t val = (data[index++] - centre - centre/2 + midpoint(predictions[best_pred[x_m]],predictions[best_pred[(x_m + width - 1) % width]]));
				out_buf[datalocation] = val % centre;
			}

			forige_TL = top_row[x_m];
			top_row[x_m] = out_buf[datalocation];
			forige = out_buf[datalocation];

			int best_val = centre*2;
			best_pred[x_m] = 0;

			if(y_m + 1 < height){
				predictor = tile_map[((y_m + 1)/tile_height) * x_tiles + (x_m/tile_width)];//future row
				for(int j=0;j<16;j++){
					int val = std::abs(out_buf[datalocation] - predictions[j]);
					if(val < best_val && (predictor & (1 << j))){
						best_val = val;
						best_pred[x_m] = j;
					}
				}
			}
		}
	}
	return out_buf;
}

#endif //UNPREDICTION_HEADER
