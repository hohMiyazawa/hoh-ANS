./simple_entropy_encoder simple_entropy_encoder.cpp tmp_compressed
./simple_entropy_decoder tmp_compressed tmp_decompressed
if cmp -s simple_entropy_encoder.cpp tmp_decompressed ; then
	echo "\033[0;32mEntropy roundtrip: OK\033[0m"
	rm tmp_compressed tmp_decompressed;
else
	echo "\033[0;31mEntropy roundtrip: FAILED\033[0m"
	# keep tmp files for diagnosis
fi
