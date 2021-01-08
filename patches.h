#ifndef PATCHES_HEADER
#define PATCHES_HEADER

#include <assert.h>

int detect_patches(uint8_t* source, size_t size, int width, int height){
	//TODO assert size = width*height
	/*uint8_t* patch_buf = new uint8_t[size];
	delete[] patch_buf;*/
	int min_size = 16;
	/*int count = 0;
	for(int i=0;i<size - min_size - min_size*width;i++){
		count++;
		for(int j=0;j<i;j++){
			int found = 0;
			for(int k=0;k<min_size;k++){
				for(int l=0;l<min_size;l++){
					if(source[i + k*width + l] != source[j + k*width + l]){
						found = 1;
						//k=min_size;
						//j=i;
						break;
					}
				}
			}
			if(found == 0){
				for(int k=0;k<min_size;k++){
					for(int l=0;l<min_size;l++){
						if(source[i + k*width + l] != source[i]){
							printf("patch %d %d\n",(int)i,(int)j);
							k=min_size;
							break;
						}
					}
				}
			}
		}
	}*/
	/*int best_area = 1;
	int best_x1 = 0;
	int best_y1 = 0;
	int best_x2 = 0;
	int best_y2 = 0;
	int best_width = 0;
	int best_height = 0;
	for(int row = 0;row < height - min_size;row++){
		for(int i=0;i<width - min_size;i++){
			for(int j=0;j<i;j++){
				if(
					i >= best_x2
					&& i <= best_x2 + best_width
					&& j >= best_x1
					&& j <= best_x1 + best_width
					&& row < best_y1
				){
					continue;
				}
				for(int patch_x = 0;patch_x + i < width && j + patch_x < i;patch_x++){
					if(source[row*width + i + patch_x] == source[row*width + j + patch_x]){
						for(int patch_y = 0;patch_y + row < height;patch_y++){
							if(source[row*width + i + patch_x + patch_y*width] == source[row*width + j + patch_x + patch_y*width]){
								int area = patch_x*patch_y;
								if(area > best_area){
									best_area = area;
									best_y1 = row;
									best_y2 = row;
									best_x1 = j;
									best_x2 = i;
									best_width = patch_x;
									best_height = patch_y;
									printf("patch %d: %d %d %d %d\n",best_area,best_x1,best_y1,best_x2,best_y2);
								}
							}
							else{
								break;
							}
						}
					}
					else{
						break;
					}
				}
			}
		}
	}
	printf("patch final %d: %d %d %d %d\n",best_area,best_x1,best_y1,best_x2,best_y2);*/

	uint32_t locations[1<<16];
	for(int i=0;i<(1<<16);i++){
		locations[i] = 0;
	}

	uint8_t* nuked = new uint8_t[size];
	for(int i=0;i<size;i++){
		nuked[i] = 0;
	}
	

	int count = 0;

	for(int row = 0;row < height - min_size;row++){
		for(int i=0;i<width - min_size;i++){
			uint16_t cumula = 0;
			int every = 1;
			int colours[8];
			colours[0] = source[row*width + i];
			for(int x=0;x<min_size;x++){
			for(int y=0;y<min_size;y++){
				cumula += source[(row + x)*width + i + y];
				if(every < 8){
					int c_found = 0;
					for(int index=0;index <= every;index++){
						if(colours[index] == source[(row + x)*width + i + y]){
							c_found = 1;
							break;
						}
					}
					if(c_found == 0){
						colours[every] = source[(row + x)*width + i + y];
						every++;
					}
				}
			}
			}
			if(every > 7){
				if(locations[cumula]){
					if(
						locations[cumula]/width + min_size < row
						|| (locations[cumula] % width) + min_size < i
						|| (locations[cumula] % width) > i + min_size
					){
						int found = 0;
						for(int x=0;x<min_size;x++){
						for(int y=0;y<min_size;y++){
							if(
								source[(row + x)*width + i + y] != source[x*width + y + locations[cumula]]
								|| nuked[x*width + y + locations[cumula]]
							){
								found = 1;
								x = min_size;
								break;
							}
							if(nuked[(row + x)*width + i + y]){
								found = 2;
								x = min_size;
								break;
							}
						}
						}
						if(found == 0){
							count++;
							for(int x=0;x<min_size;x++){
							for(int y=0;y<min_size;y++){
								nuked[(row + x)*width + i + y] = 1;
								source[(row + x)*width + i + y] = 0;
							}
							}
						}
						//printf("l %d %d\n",locations[cumula],row*width + i);
						if(found != 2){
							locations[cumula] = row*width + i;
						}
					}
				}
				else{
					locations[cumula] = row*width + i;
				}
			}
		}
	}
	/*uint32_t matches[1<<16];
	for(int i=0;i<(1<<16);i++){
		matches[i] = 0;
	}
	for(int row = 0;row < height - min_size;row++){
		for(int i=0;i<width - min_size;i++){
			matches[hash_buf[row*width + i]]++;
		}
	}
	for(int i=0;i<(1<<16);i++){
		if(matches[i] > 1){
			count++;
		}
	}*/
	printf("16x16 patches found: %d\n",count);

	delete[] nuked;

	return count;
}

int detect_patches(uint16_t* source, size_t size, int width, int height){
	int min_size = 16;

	uint32_t locations[1<<16];
	for(int i=0;i<(1<<16);i++){
		locations[i] = 0;
	}

	uint8_t* nuked = new uint8_t[size];
	for(int i=0;i<size;i++){
		nuked[i] = 0;
	}
	

	int count = 0;

	for(int row = 0;row < height - min_size;row++){
		for(int i=0;i<width - min_size;i++){
			uint16_t cumula = 0;
			int every = 1;
			int colours[8];
			colours[0] = source[row*width + i];
			for(int x=0;x<min_size;x++){
			for(int y=0;y<min_size;y++){
				cumula += source[(row + x)*width + i + y];
				if(every < 8){
					int c_found = 0;
					for(int index=0;index <= every;index++){
						if(colours[index] == source[(row + x)*width + i + y]){
							c_found = 1;
							break;
						}
					}
					if(c_found == 0){
						colours[every] = source[(row + x)*width + i + y];
						every++;
					}
				}
			}
			}
			if(every > 7){
				if(locations[cumula]){
					if(
						locations[cumula]/width + min_size < row
						|| (locations[cumula] % width) + min_size < i
						|| (locations[cumula] % width) > i + min_size
					){
						int found = 0;
						for(int x=0;x<min_size;x++){
						for(int y=0;y<min_size;y++){
							if(
								source[(row + x)*width + i + y] != source[x*width + y + locations[cumula]]
								|| nuked[x*width + y + locations[cumula]]
							){
								found = 1;
								x = min_size;
								break;
							}
							if(nuked[(row + x)*width + i + y]){
								found = 2;
								x = min_size;
								break;
							}
						}
						}
						if(found == 0){
							count++;
							for(int x=0;x<min_size;x++){
							for(int y=0;y<min_size;y++){
								nuked[(row + x)*width + i + y] = 1;
								source[(row + x)*width + i + y] = 0;
							}
							}
						}
						//printf("l %d %d\n",locations[cumula],row*width + i);
						if(found != 2){
							locations[cumula] = row*width + i;
						}
					}
				}
				else{
					locations[cumula] = row*width + i;
				}
			}
		}
	}
	printf("16x16 patches found: %d\n",count);

	delete[] nuked;

	return count;
}

#endif // PATCHES_HEADER
