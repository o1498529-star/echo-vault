cmake_minimum_required(VERSION 3.22.1)
project(echovault)
add_library(echovault SHARED game.cpp)
# Small, fast native code: no exceptions/RTTI, unused code stripped, 16 KB page alignment (required for newer Android devices).
target_compile_options(echovault PRIVATE -O2 -Wall -fno-exceptions -fno-rtti -fvisibility=hidden -ffunction-sections -fdata-sections)
target_link_options(echovault PRIVATE -Wl,--gc-sections -Wl,--build-id=none -Wl,-z,max-page-size=16384)
