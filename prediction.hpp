#ifndef PREDICTION_HEADER
#define PREDICTION_HEADER

#include "predictor_operations.hpp"

uint16_t* channelpredict_fastpath(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	size_t* buffer_size
){
	int centre = 1<<depth;
	uint16_t* out_buf = new uint16_t[width*height];

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
			uint16_t L = forige;
			uint16_t T = top_row[x_m];
			uint16_t TL = forige_TL;

			out_buf[index++] = (data[datalocation] - median(T,L,T+L-TL) + centre/2 + centre) % centre;

			forige_TL = top_row[x_m];
			top_row[x_m] = data[datalocation];
			forige = data[datalocation];
		}
	}
	*buffer_size = index;
	return out_buf;
}

uint16_t* channelpredict_section(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	int x_tiles,
	int y_tiles,
	int x,
	int y,
	uint16_t predictor,
	size_t* buffer_size
){
	if(predictor == 0b0000000000010000 && x_tiles == 1 && y_tiles == 1 && x == 0 && y == 0){
		return channelpredict_fastpath(
			data,
			size,
			width,
			height,
			depth,
			buffer_size
		);
	}
	int centre = 1<<depth;
	int tile_width  = (width  + x_tiles - 1)/x_tiles;
	int tile_height = (height + y_tiles - 1)/y_tiles;
	int x_coord = x*tile_width;
	int y_coord = y*tile_height;
	uint16_t* out_buf = new uint16_t[tile_width*tile_height];

	int best_pred[tile_width];//approximate
	for(int i=0;i<tile_width;i++){
		best_pred[i] = 4;
	}

	uint16_t top_row[tile_width];
	uint16_t forige;
	uint16_t forige_TL;

	if(y){
		for(int i=0;i<tile_width;i++){
			top_row[i] = data[y_coord*width + x_coord + i - width];
		}
	}
	else{
		for(int i=0;i<tile_width;i++){
			top_row[i] = centre/2;
		}
	}
	size_t index = 0;
	for(int y_m=0;y_m < tile_height && y_coord + y_m < height;y_m++){
		if(x){
			forige = data[(y_coord + y_m)*width + x_coord - 1];
			if(y_m || y){
				forige_TL = data[(y_coord + y_m - 1)*width + x_coord - 1];
			}
			else{
				forige_TL = centre/2;
			}
		}
		else{
			forige    = centre/2;
			forige_TL = centre/2;
		}
		for(int x_m=0;x_m < tile_width && x_coord + x_m < width;x_m++){
			int datalocation = (y_coord + y_m)*width + x_coord + x_m;
			uint16_t L = forige;
			uint16_t T = top_row[x_m];
			uint16_t TL = forige_TL;
			uint16_t TR = top_row[(x_m + tile_width + 1) % tile_width];
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
			out_buf[index++] = (data[datalocation] - midpoint(predictions[best_pred[x_m]],predictions[best_pred[(x_m + tile_width - 1) % tile_width]]) + centre/2 + centre) % centre;
			forige_TL = top_row[x_m];
			top_row[x_m] = data[datalocation];
			forige = data[datalocation];
			int best_val = centre*2;
			best_pred[x_m] = 0;
			for(int j=0;j<16;j++){
				int val = std::abs(data[datalocation] - predictions[j]);
				if(val < best_val && (predictor & (1 << j))){
					best_val = val;
					best_pred[x_m] = j;
				}
			}
		}
	}
	*buffer_size = index;
	return out_buf;
}

uint16_t* channelpredict_all(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	int x_tiles,
	int y_tiles,
	uint16_t* tile_map
){
	int centre = 1<<depth;
	int tile_width  = (width  + x_tiles - 1)/x_tiles;
	int tile_height = (height + y_tiles - 1)/y_tiles;
	uint16_t* out_buf = new uint16_t[size];

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
			uint16_t prediction;
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
			out_buf[index++] = (data[datalocation] - midpoint(predictions[best_pred[x_m]],predictions[best_pred[(x_m + width - 1) % width]]) + centre/2 + centre) % centre;
			forige_TL = top_row[x_m];
			top_row[x_m] = data[datalocation];
			forige = data[datalocation];

			int best_val = centre*2;
			best_pred[x_m] = 0;

			if(y_m + 1 < height){
				predictor = tile_map[((y_m + 1)/tile_height) * x_tiles + (x_m/tile_width)];//future row
				for(int j=0;j<16;j++){
					int val = std::abs(data[datalocation] - predictions[j]);
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

#endif //PREDICTION_HEADER
