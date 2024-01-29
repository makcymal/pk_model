cmake_minimum_required(VERSION 3.13)
project(maxflow)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES maxflow.cpp pk_model/abbreviations.txt pk_model/graph_edges.txt pk_model/graph_popul.txt)

add_executable(maxflow ${SOURCE_FILES})