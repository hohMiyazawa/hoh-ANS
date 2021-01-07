#ifndef SYMBOLSTATS_HEADER
#define SYMBOLSTATS_HEADER

#include <cmath>

struct SymbolStats_256{
	uint32_t freqs[256];
	uint32_t cum_freqs[257];

	void count_freqs(uint8_t const* in, size_t nbytes);
	void calc_cum_freqs();
	void normalize_freqs(uint32_t target_total);
};

struct SymbolStats_512{
	uint32_t freqs[512];
	uint32_t cum_freqs[513];

	void count_freqs(uint16_t const* in, size_t nbytes);
	void calc_cum_freqs();
	void normalize_freqs(uint32_t target_total);
	void laplace(float deviation,uint32_t target_sum);
	float deviation();
};

struct SymbolStats_1024{
	uint32_t freqs[1024];
	uint32_t cum_freqs[1025];

	void count_freqs(uint16_t const* in, size_t nbytes);
	void calc_cum_freqs();
	void normalize_freqs(uint32_t target_total);
	void laplace(float deviation,uint32_t target_sum);
};

void SymbolStats_512::laplace(float deviation,uint32_t target_sum){
	for (int i=0; i < 512; i++){
		freqs[i] = (uint32_t) (target_sum*std::exp(-std::abs(256 - i)/deviation)/(2*deviation));
	}
}

void SymbolStats_1024::laplace(float deviation,uint32_t target_sum){
	for (int i=0; i < 1024; i++){
		freqs[i] = (uint32_t) (target_sum*std::exp(-std::abs(512 - i)/deviation)/(2*deviation));
	}
}

float SymbolStats_512::deviation(){
	long sum = 0;
	long diffs = 0;
	for(int i=0;i<512;i++){
		diffs += (256 - i)*(256 - i) * freqs[i];
		sum += freqs[i];
	}
	return (float)std::sqrt((double)diffs/(double)sum);
}

void SymbolStats_256::count_freqs(uint8_t const* in, size_t nbytes){
	for (int i=0; i < 256; i++)
		freqs[i] = 0;

	for (size_t i=0; i < nbytes; i++)
		freqs[in[i]]++;
}

void SymbolStats_512::count_freqs(uint16_t const* in, size_t nbytes){
	for (int i=0; i < 512; i++)
		freqs[i] = 0;

	for (size_t i=0; i < nbytes; i++)
		freqs[in[i]]++;
}

void SymbolStats_1024::count_freqs(uint16_t const* in, size_t nbytes){
	for (int i=0; i < 1024; i++)
		freqs[i] = 0;

	for (size_t i=0; i < nbytes; i++)
		freqs[in[i]]++;
}

void SymbolStats_256::calc_cum_freqs(){
	cum_freqs[0] = 0;
	for (int i=0; i < 256; i++)
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
}

void SymbolStats_512::calc_cum_freqs(){
	cum_freqs[0] = 0;
	for (int i=0; i < 512; i++)
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
}

void SymbolStats_1024::calc_cum_freqs(){
	cum_freqs[0] = 0;
	for (int i=0; i < 1024; i++)
		cum_freqs[i+1] = cum_freqs[i] + freqs[i];
}

void SymbolStats_256::normalize_freqs(uint32_t target_total){
	assert(target_total >= 256);
	
	calc_cum_freqs();
	uint32_t cur_total = cum_freqs[256];
	
	// resample distribution based on cumulative freqs
	for (int i = 1; i <= 256; i++)
		cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;

	// if we nuked any non-0 frequency symbol to 0, we need to steal
	// the range to make the frequency nonzero from elsewhere.
	//
	// this is not at all optimal, i'm just doing the first thing that comes to mind.
	for (int i=0; i < 256; i++) {
		if (freqs[i] && cum_freqs[i+1] == cum_freqs[i]) {
			// symbol i was set to zero freq

			// find best symbol to steal frequency from (try to steal from low-freq ones)
			uint32_t best_freq = ~0u;
			int best_steal = -1;
			for (int j=0; j < 256; j++) {
				uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
				if (freq > 1 && freq < best_freq) {
					best_freq = freq;
					best_steal = j;
				}
			}
			assert(best_steal != -1);

			// and steal from it!
			if (best_steal < i) {
				for (int j = best_steal + 1; j <= i; j++)
					cum_freqs[j]--;
			} else {
				assert(best_steal > i);
				for (int j = i + 1; j <= best_steal; j++)
					cum_freqs[j]++;
			}
		}
	}

	// calculate updated freqs and make sure we didn't screw anything up
	assert(cum_freqs[0] == 0 && cum_freqs[256] == target_total);
	for (int i=0; i < 256; i++) {
		if (freqs[i] == 0)
			assert(cum_freqs[i+1] == cum_freqs[i]);
		else
			assert(cum_freqs[i+1] > cum_freqs[i]);

		// calc updated freq
		freqs[i] = cum_freqs[i+1] - cum_freqs[i];
	}
}

void SymbolStats_512::normalize_freqs(uint32_t target_total){
	assert(target_total >= 512);
	
	calc_cum_freqs();
	uint32_t cur_total = cum_freqs[512];
	
	// resample distribution based on cumulative freqs
	for (int i = 1; i <= 512; i++)
		cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;

	// if we nuked any non-0 frequency symbol to 0, we need to steal
	// the range to make the frequency nonzero from elsewhere.
	//
	// this is not at all optimal, i'm just doing the first thing that comes to mind.
	for (int i=0; i < 512; i++) {
		if (freqs[i] && cum_freqs[i+1] == cum_freqs[i]) {
			// symbol i was set to zero freq

			// find best symbol to steal frequency from (try to steal from low-freq ones)
			uint32_t best_freq = ~0u;
			int best_steal = -1;
			for (int j=0; j < 512; j++) {
				uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
				if (freq > 1 && freq < best_freq) {
					best_freq = freq;
					best_steal = j;
				}
			}
			assert(best_steal != -1);

			// and steal from it!
			if (best_steal < i) {
				for (int j = best_steal + 1; j <= i; j++)
					cum_freqs[j]--;
			} else {
				assert(best_steal > i);
				for (int j = i + 1; j <= best_steal; j++)
					cum_freqs[j]++;
			}
		}
	}

	// calculate updated freqs and make sure we didn't screw anything up
	assert(cum_freqs[0] == 0 && cum_freqs[512] == target_total);
	for (int i=0; i < 512; i++) {
		if (freqs[i] == 0)
			assert(cum_freqs[i+1] == cum_freqs[i]);
		else
			assert(cum_freqs[i+1] > cum_freqs[i]);

		// calc updated freq
		freqs[i] = cum_freqs[i+1] - cum_freqs[i];
	}
}

void SymbolStats_1024::normalize_freqs(uint32_t target_total){
	assert(target_total >= 1024);
	
	calc_cum_freqs();
	uint32_t cur_total = cum_freqs[1024];
	
	// resample distribution based on cumulative freqs
	for (int i = 1; i <= 1024; i++)
		cum_freqs[i] = ((uint64_t)target_total * cum_freqs[i])/cur_total;

	// if we nuked any non-0 frequency symbol to 0, we need to steal
	// the range to make the frequency nonzero from elsewhere.
	//
	// this is not at all optimal, i'm just doing the first thing that comes to mind.
	for (int i=0; i < 1024; i++) {
		if (freqs[i] && cum_freqs[i+1] == cum_freqs[i]) {
			// symbol i was set to zero freq

			// find best symbol to steal frequency from (try to steal from low-freq ones)
			uint32_t best_freq = ~0u;
			int best_steal = -1;
			for (int j=0; j < 1024; j++) {
				uint32_t freq = cum_freqs[j+1] - cum_freqs[j];
				if (freq > 1 && freq < best_freq) {
					best_freq = freq;
					best_steal = j;
				}
			}
			assert(best_steal != -1);

			// and steal from it!
			if (best_steal < i) {
				for (int j = best_steal + 1; j <= i; j++)
					cum_freqs[j]--;
			} else {
				assert(best_steal > i);
				for (int j = i + 1; j <= best_steal; j++)
					cum_freqs[j]++;
			}
		}
	}

	// calculate updated freqs and make sure we didn't screw anything up
	assert(cum_freqs[0] == 0 && cum_freqs[1024] == target_total);
	for (int i=0; i < 1024; i++) {
		if (freqs[i] == 0)
			assert(cum_freqs[i+1] == cum_freqs[i]);
		else
			assert(cum_freqs[i+1] > cum_freqs[i]);

		// calc updated freq
		freqs[i] = cum_freqs[i+1] - cum_freqs[i];
	}
}

#endif // SYMBOLSTATS_HEADER
