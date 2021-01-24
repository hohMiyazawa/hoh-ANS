#include <string.h>
#include <stdint.h>

#include "varint.hpp"
#include "file_io.hpp"
#include "entropy_encoding.hpp"

/* simple program to test the implementations of 
	entropy_encoding.hpp
*/
void print_usage(){
	printf("usage: simple_entropy_encoder infile outfile\n");

}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);
	printf("read %d bytes\n",(int)in_size);
	uint8_t* output_bytes = new uint8_t[in_size + 256];
	size_t output_size = encode_entropy(
		in_bytes,
		in_size,
		256,
		output_bytes,
		12,
		1//iagnostic
	);

	delete[] in_bytes;
	write_file(argv[2],output_bytes,output_size);
	printf("wrote %d bytes\n",(int)output_size);
	delete[] output_bytes;
	return 0;
}
