cmake_minimum_required(VERSION 3.15)
project(user_threads C)

set(CMAKE_C_STANDARD 99)

add_library(user_threads library.c library.h)