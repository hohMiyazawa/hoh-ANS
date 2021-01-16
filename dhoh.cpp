#include "platform.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <cstdlib>
#include <cmath>
#include <math.h>

#include "rans64.hpp"
#include "file_io.hpp"
#include "symbolstats.hpp"
#include "channel.hpp"
#include "lz.hpp"
#include "channel_encode.hpp"
#include "bitimage.hpp"

void print_usage(){
	printf("usage: dhoh infile.hoh outfile.rgb\n");
	printf("The output file consist of raw 8bit RGB bytes\n");
	printf("You can turn that into an image with imagemagick:\n");
	printf("[TODO: imagemagick command]\n");
}

int main(int argc, char *argv[]){
	printf("Not ready for actual usage!\n");
	if(argc == 2 && (strcmp(argv[1],"--help") == 0 || strcmp(argv[1],"-h") == 0)){
		print_usage();
		return 0;//technically everything went OK
	}
	else if(argc == 2 && strcmp(argv[1],"--version") == 0){
		printf("experimental software\n");
		return 0;
	}
	else if(argc < 3){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}
	return 0;
}
