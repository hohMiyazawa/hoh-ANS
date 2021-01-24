#ifndef LAYER_ENCODE_HEADER
#define LAYER_ENCODE_HEADER

#include <cmath>
#include <math.h>

#include "entropy_encoding.hpp"
#include "entropy_decoding.hpp"//remove, only for debugging
#include "prediction.hpp"

size_t layer_encode(
	uint16_t* data,
	size_t size,
	int width,
	int height,
	int depth,
	int cruncher_mode,
	uint8_t* LEMPEL_NUKE,
	uint8_t* compressed
){
	size_t output_index = 0;
	size_t possible_size = (depth*size + depth*size % 8 + 1024)/8;

	size_t compaction_overhead = 0;

//disabled until parser ready
/*
	if(cruncher_mode){
		uint32_t colused[1<<depth];
		for(int i=0;i<(1<<depth);i++){
			colused[i] = 0;
		}
		for(int i=0;i<size;i++){
			colused[data[i]]++;
		}
		int col_count = 0;
		for(int i=0;i<(1<<depth);i++){
			if(colused[i]){
				col_count++;
			}
		}
		if(col_count < 32){
			compaction_overhead = (1<<depth)/8;
			size_t next_open = 0;
			uint32_t colmap[1<<depth];
			for(int i=0;i<(1<<depth);i++){
				if(colused[i]){
					colmap[i] = next_open++;
				}
			}
			for(int i=0;i<size;i++){
				data[i] = colmap[data[i]];
			}
		}
	}
*/
	compressed[output_index++] = (1<<4)/*use prediction*/ + 0b00000000/*no compaction*/;

	uint32_t prob_bits = 15;

	uint32_t* dummy1;
	uint32_t* dummy2;
	uint32_t* dummy3;

	size_t chunk_size;

	uint16_t* predict = channelpredict_section(
		data,
		size,
		width,
		height,
		depth,
		1,
		1,
		0,
		0,
		0b0000000000010000,
		&chunk_size
	);
	/*{
		uint32_t colused[1<<depth];
		for(int i=0;i<(1<<depth);i++){
			colused[i] = 0;
		}
		for(int i=0;i<size;i++){
			colused[predict[i]]++;
		}
		printf(".......\n");
		for(int i=0;i<(1<<depth);i++){
			printf("%d \n",colused[i]);
		}
	}*/

	uint16_t* predict_cleaned;
	size_t cleaned_pointer;

	predict_cleaned = new uint16_t[size];
	cleaned_pointer = 0;
	for(int i=0;i<size;i++){
		if(LEMPEL_NUKE[i] == 0){
			predict_cleaned[cleaned_pointer++] = predict[i];
		}
	}

	size_t maximum_size = 10 + (1<<(depth))*2 + (cleaned_pointer*(depth) + 8 - 1)/8;

	uint8_t* dummyrand = new uint8_t[maximum_size];
	uint8_t* permanent = new uint8_t[maximum_size];

	size_t channel_size_lz = encode_entropy(
		predict_cleaned,
		cleaned_pointer,
		1<<(depth),
		dummyrand,
		prob_bits,
		0//no diagnostic
	);

	if(channel_size_lz < possible_size){
		possible_size = channel_size_lz;
		uint8_t* tmp = permanent;
		permanent = dummyrand;
		dummyrand = tmp;
	}

	int total_tiles = 1;
	int tile_cost = 2;

	int grid_size = 40;//heuristic

	if(
		cruncher_mode
		&& (
			(width + grid_size - 1)/grid_size > 1
			|| (height + grid_size - 1)/grid_size > 1
		)
	){
		int freqs[1<<(depth)];
		for(int i=0;i < (1<<(depth));i++){
			freqs[i] = 1;//to have no zero values, so the frequency table is usable for other predictors
		}
		for(int i=0;i<size;i++){
			freqs[predict[i]]++;
		}
		double entropy[1<<(depth)];
		for(int i=0;i < (1<<(depth));i++){
			entropy[i] = -std::log2((double)freqs[i]/(double)size);
		}
		double total_entropy = 0;
		for(int i=0;i < (1<<(depth));i++){
			total_entropy += entropy[i] * (freqs[i] - 1);
		}

		int x_tiles = (width + grid_size - 1)/grid_size;
		int y_tiles = (height + grid_size - 1)/grid_size;
		total_tiles = x_tiles*y_tiles;
		uint16_t* predictor_list = new uint16_t[total_tiles];
		uint8_t* predictor_index_list = new uint8_t[total_tiles];
		double new_cost = 0;

		uint16_t masks[14] = {
			0b0000000000000001,
			0b0000000000000010,
			0b0000000000100000,
			0b0000000000010000,
			0b1111111110111111,//good
			0b0000000000000011,
			0b1111111111111101,
			0b1111111111111011,
			0b1111111111110111,
			0b1111111111101111,
			0b1111111111011111,
			0b1111111101111111,
			0b1111110111111111,

			0b1111111111111111
		};
		for(int i=0;i<total_tiles;i++){
			double tile_cost = 99999999999;//yea, fix this later
			for(int pred=0;pred<14 && pred < cruncher_mode*5;pred++){
				uint16_t* tile_predict = channelpredict_section(
					data,
					size,
					width,
					height,
					depth,
					x_tiles,
					y_tiles,
					i % x_tiles,
					i / x_tiles,
					masks[pred],
					&chunk_size
				);
				double current_cost = 0;
				for(int val=0;val<chunk_size;val++){
					current_cost += entropy[tile_predict[val]];
				}
				if(current_cost < tile_cost){
					tile_cost = current_cost;
					predictor_list[i] = masks[pred];
					predictor_index_list[i] = pred;
				}
			}
			new_cost += tile_cost;
		}

		predict = channelpredict_all(
			data,
			size,
			width,
			height,
			depth,
			x_tiles,
			y_tiles,
			predictor_list
		);
		if(cruncher_mode > 2){//refine estimate

			for(int i=0;i < (1<<(depth));i++){
				freqs[i] = 1;
			}
			for(int i=0;i<size;i++){
				freqs[predict[i]]++;
			}
			for(int i=0;i < (1<<(depth));i++){
				entropy[i] = -std::log2((double)freqs[i]/(double)size);
			}
			total_entropy = 0;
			for(int i=0;i < (1<<(depth));i++){
				total_entropy += entropy[i] * (freqs[i] - 1);
			}
			//printf("total entropy 3 %f\n",total_entropy/8);

			new_cost = 0;
			for(int i=0;i<total_tiles;i++){
				double tile_cost = 99999999999;//yea, fix this later
				for(int pred=0;pred<14 && pred < cruncher_mode*5;pred++){
					uint16_t* tile_predict = channelpredict_section(
						data,
						size,
						width,
						height,
						depth,
						x_tiles,
						y_tiles,
						i % x_tiles,
						i / x_tiles,
						masks[pred],
						&chunk_size
					);
					double current_cost = 0;
					for(int val=0;val<chunk_size;val++){
						current_cost += entropy[tile_predict[val]];
					}
					if(current_cost < tile_cost){
						tile_cost = current_cost;
						predictor_list[i] = masks[pred];
						predictor_index_list[i] = pred;
					}
				}
				new_cost += tile_cost;
			}
			//printf("total entropy 4 %f\n",new_cost/8);
			predict = channelpredict_all(
				data,
				size,
				width,
				height,
				depth,
				x_tiles,
				y_tiles,
				predictor_list
			);
		}
		delete[] predictor_list;
		//printf("tiles %d\n",total_tiles);

		tile_cost = 4 + (total_tiles)/2;

		compressed[output_index++] = x_tiles-1;
		compressed[output_index++] = y_tiles-1;

		uint8_t masks_used[14];
		for(int j=0;j<14;j++){
			masks_used[j] = 0;
		}
		for(int i=0;i<total_tiles;i++){
			masks_used[predictor_index_list[i]] = 1;
		}
		uint8_t used_predictors = 0;
		for(int j=0;j<14;j++){
			used_predictors += masks_used[j];
		}
//
		compressed[output_index++] = used_predictors;
		for(int j=0;j<14;j++){
			if(masks_used[j]){
				compressed[output_index++] = (uint8_t)(masks[j]>>8);
				compressed[output_index++] = (uint8_t)(masks[j] % 256);
			}
		}
		uint8_t mapper[14];
		uint8_t counter = 0;
		for(int j=0;j<14;j++){
			if(masks_used[j]){
				mapper[j] = counter++;
			}
		}
		for(size_t i=0;i<total_tiles;i++){
			predictor_index_list[i] = mapper[predictor_index_list[i]];
		}
		size_t predictor_map_size = encode_entropy(
			predictor_index_list,
			0,
			14,
			compressed + output_index,
			8,
			0//no diagnostic
		);
		output_index += predictor_map_size;
		delete[] predictor_index_list;
	}
	else{
		compressed[output_index++] = 0;//x tiles-1
		compressed[output_index++] = 0;//y tiles-1
		compressed[output_index++] = 0b00000000;//FFV1 predictor upper bits
		compressed[output_index++] = 0b00010000;//FFV1 predictor lower bits
	}
	if(cruncher_mode){
		size_t temp_size;
		cleaned_pointer = 0;
		for(int j=0;j<size;j++){
			if(LEMPEL_NUKE[j] == 0){
				predict_cleaned[cleaned_pointer++] = predict[j];
			}
		}
		size_t temp_size1 = encode_entropy(
			predict_cleaned,
			cleaned_pointer,
			1<<(depth),
			dummyrand,
			16,
			0//no diagnostic
		);
		size_t temp_size2 = encode_entropy(
			predict_cleaned,
			cleaned_pointer,
			1<<(depth),
			dummyrand,
			15,
			0//no diagnostic
		);
		if(temp_size1 < temp_size2){
			if(temp_size1 < possible_size){
				possible_size = temp_size1;
			}
			for(uint32_t i=17;i < 20;i++){
				temp_size = encode_entropy(
					predict_cleaned,
					cleaned_pointer,
					1<<(depth),
					dummyrand,
					i,
					0//no diagnostic
				);
				if(temp_size < possible_size){
					possible_size = temp_size;
					uint8_t* tmp = permanent;
					permanent = dummyrand;
					dummyrand = tmp;
				}
			}
		}
		else{
			if(temp_size2 < possible_size){
				possible_size = temp_size2;
			}
			for(uint32_t i=14;i > 11;i--){
				temp_size = encode_entropy(
					predict_cleaned,
					cleaned_pointer,
					1<<(depth),
					dummyrand,
					i,
					0//no diagnostic
				);
				if(temp_size < possible_size){
					possible_size = temp_size;
					uint8_t* tmp = permanent;
					permanent = dummyrand;
					dummyrand = tmp;
				}
			}
		}
	}
	delete[] dummyrand;
	delete[] predict;
	delete[] predict_cleaned;
	for(size_t i=0;i<possible_size;i++){
		compressed[output_index++] = permanent[i];
	}
	/*size_t symbol_size_d;
	size_t byte_pointer = 0;
	uint16_t* symbols_d = decode_entropy(
		permanent,
		possible_size,
		&byte_pointer,
		&symbol_size_d,
		1//diagnostic
	);
	delete[] symbols_d;*/
	delete[] permanent;

	return output_index;
}

#endif // LAYER_ENCODE_HEADER
