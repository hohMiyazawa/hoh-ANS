echo "rgb encoding..."
./choh example.rgb tmp_compressed 2 2 -s1
echo "rgb decoding..."
./dhoh tmp_compressed tmp_decompressed
if cmp -s example.rgb tmp_decompressed ; then
	echo "\033[0;32mRGB roundtrip: OK\033[0m"
	rm tmp_compressed tmp_decompressed;
else
	echo "\033[0;31mRGB roundtrip: FAILED\033[0m"
	# keep tmp files for diagnosis
fi
