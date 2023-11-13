cmake_minimum_required(VERSION 3.0)

project(TimeWheel)

add_compile_options(-g -std=c++14)

include_directories(./include)

add_library(lizyTimeWheel SHARED
  ./src/timewheel.cc
)

target_link_libraries(lizyTimeWheel
  pthread
)