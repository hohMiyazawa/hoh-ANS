Experimental lossless image compression, based on rANS entropy coding ([ryg-rans](https://github.com/rygorous/ryg_rans)) and traditional prediction and colour space transforms.

Proof of concept, useless for most edge cases.

## Usage

```
hoh infile.raw width height
```
"infile.raw" must consist of raw 8bit RGB bytes.
You can make such a file with imagemagick:
```
```
convert input.png -depth 8 rgb:infile.raw
```
Encoded size is reported, but no data is written.

## Building

```
make
```

That's all you should need. Requires GNU make and gcc.
Only tested on 64bit x86.
