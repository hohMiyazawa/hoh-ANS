Experimental lossless image compression, based on rANS entropy coding ([ryg-rans](https://github.com/rygorous/ryg_rans)) and traditional prediction and colour space transforms.

Proof of concept, useless for most edge cases.

## Usage
Encoding:
```
choh infile.rgb outfile.hoh width height -sN
```
Where "N" is a number 0-4 (fast-slow).
"infile.rgb" must consist of raw 8bit RGB bytes.
You can make such a file with imagemagick:
```
convert input.png -depth 8 rgb:infile.rgb
```
Encoded size is reported, but no data is written. (Work in progress, it currently writes headers and stuff).

Decoding:
```
dhoh infile.hoh outfile.rgb
```

## Building

```
make
```

That's all you should need. Requires GNU make and gcc.
Only tested on 64bit x86.
