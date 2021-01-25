#ifndef STATTOOLS_HEADER
#define STATTOOLS_HEADER

#include <assert.h>

void calc_cum_freqs(uint32_t* freqs,uint32_t* cum_freqs, size_t size){
	cum_freqs[0] = 0;
	for (size_t i=0; i < size; i++){
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
	}
}

void normalize_freqs(uint32_t* freqs,uint32_t* cum_freqs, size_t size,uint32_t target_total){
	assert(target_total >= size);
	
	calc_cum_freqs(freqs,cum_freqs,size);

	uint32_t cur_total = cum_freqs[size];
	
	// resample distribution based on cumulative freqs
	for (size_t i = 1; i <= size; i++)
		cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;

	// if we nuked any non-0 frequency symbol to 0, we need to steal
	// the range to make the frequency nonzero from elsewhere.
	//
	// this is not at all optimal, i'm just doing the first thing that comes to mind.
	for(size_t i=0; i < size; i++) {
		if(freqs[i] && cum_freqs[i+1] == cum_freqs[i]){
			// symbol i was set to zero freq

			// find best symbol to steal frequency from (try to steal from low-freq ones)
			uint32_t best_freq = ~0u;
			int best_steal = -1;
			for(size_t j=0; j < size; j++){
				uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
				if (freq > 1 && freq < best_freq) {
					best_freq = freq;
					best_steal = j;
				}
			}
			assert(best_steal != -1);

			// and steal from it!
			if(best_steal < (int)i){
				for(size_t j = best_steal + 1; j <= i; j++){
					cum_freqs[j]--;
				}
			}
			else{
				assert(best_steal > (int)i);
				for(int j = i + 1; j <= best_steal; j++){
					cum_freqs[j]++;
				}
			}
		}
	}

	// calculate updated freqs and make sure we didn't screw anything up
	assert(cum_freqs[0] == 0 && cum_freqs[size] == target_total);
	for (size_t i=0; i < size; i++) {
		if (freqs[i] == 0)
			assert(cum_freqs[i+1] == cum_freqs[i]);
		else
			assert(cum_freqs[i+1] > cum_freqs[i]);

		// calc updated freq
		freqs[i] = cum_freqs[i+1] - cum_freqs[i];
	}
}

/*
void SymbolStats_512::laplace(float deviation,uint32_t target_sum){
	for (int i=0; i < 512; i++){
		freqs[i] = (uint32_t) (target_sum*std::exp(-std::abs(256 - i)/deviation)/(2*deviation));
	}
}
*/

#endif //STATTOOLS_HEADER
