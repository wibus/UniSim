cmake_minimum_required(VERSION 3.2)

project(bruneton)

include_directories(../glew/include)

file(GLOB BRUNETON_MATH_SRC
     "math/*.cc"
)

add_library(bruneton
    constants.h
    definitions.glsl
    functions.glsl
    model.h
    model.cpp
    ${BRUNETON_MATH_SRC}
)
