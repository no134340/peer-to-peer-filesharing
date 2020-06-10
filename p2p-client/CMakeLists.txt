cmake_minimum_required(VERSION 3.7)
project(p2p_client C)

set(CMAKE_C_STANDARD 99)

add_executable(p2p_client main.c src/jsonFunctions.h src/jsonFunctions.c include/parson.c include/parson.h src/client_thread.c src/client_thread.h src/server_thread.c src/server_thread.h src/TrashQueue.c src/TrashQueue.h src/includes.h include/list.h include/list.c src/ClientTrheadSrc/ListLocalFiles.c src/ClientTrheadSrc/ListLocalFiles.h src/ClientTrheadSrc/DownloadFile.c src/ClientTrheadSrc/DownloadFile.h src/ClientTrheadSrc/CreateMetaFile.c src/ClientTrheadSrc/CreateMetaFile.h src/sha256sum.c src/sha256sum.h)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
