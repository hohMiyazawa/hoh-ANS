./simple_entropy_encoder choh.cpp tmp
./simple_entropy_decoder tmp tmp2
if cmp -s tmp tmp2 ; then
	echo "\033[0;32mEntropy roundtrip: OK\033[0m"
else
	echo "\033[0;31mEntropy roundtrip: FAILED\033[0m"
fi
rm tmp tmp2
