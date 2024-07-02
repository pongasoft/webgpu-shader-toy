#!/bin/bash

cd "$(dirname "$0")"
PWD=`pwd`
FONTS_DIR=$PWD/../fonts

# generate IconsFontWGPUShaderToy.h
python3 gen-font.py

# move it to proper location
mv IconsFontWGPUShaderToy.h $FONTS_DIR/src

# generate IconsFontWGPUShaderToy.cpp (requires binary_to_compressed_c to be in path)
binary_to_compressed_c -base85 $PWD/src/webfonts/fa-solid-900.ttf IconsFontWGPUShaderToy > $FONTS_DIR/src/IconsFontWGPUShaderToy.cpp
