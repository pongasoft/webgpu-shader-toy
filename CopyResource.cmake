cmake_minimum_required(VERSION 3.28)

message(STATUS "Copying resource SRC=${SRC} | DST=${DST} | version=${wgpu-shader-toy_VERSION}")

configure_file(${SRC} ${DST} @ONLY)
