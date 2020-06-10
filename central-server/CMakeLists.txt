cmake_minimum_required(VERSION 3.7)
project(central_server C)

set(CMAKE_C_STANDARD 99)


add_executable(central_server  include/parson.c include/parson.h src/jsonFunctions.c src/jsonFunctions.h src/list.c src/list_iterator.c src/list_node.c src/list_functions.h src/list_functions.c src/getch.c main.c)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
