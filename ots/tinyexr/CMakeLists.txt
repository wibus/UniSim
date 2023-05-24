cmake_minimum_required(VERSION 3.2)

project(tinyexr)

include_directories(miniz)

add_library(tinyexr
    tinyexr.cc
    miniz/miniz.c
)
