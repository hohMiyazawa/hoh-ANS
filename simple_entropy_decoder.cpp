#include <string.h>
#include <stdint.h>

#include "varint.hpp"
#include "file_io.hpp"
#include "entropy_decoding.hpp"

/* simple program to test the implementations of 
	entropy_decoding.hpp
*/
void print_usage(){
	printf("usage: simple_entropy_decoder infile outfile\n");

}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	size_t in_size;
	size_t byte_pointer = 0;
	uint8_t* in_bytes = read_file(argv[1], &in_size);
	printf("read %d bytes\n",(int)in_size);
	size_t output_size;
	uint8_t* output_bytes = decode_entropy_8bit(
		in_bytes,
		in_size,
		&byte_pointer,
		&output_size
	);
	delete[] in_bytes;
	write_file(argv[2],output_bytes,output_size);
	printf("wrote %d bytes\n",(int)output_size);
	delete[] output_bytes;
	return 0;
}
