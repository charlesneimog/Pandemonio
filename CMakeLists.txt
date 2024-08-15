cmake_minimum_required(VERSION 3.25)
project(Moteto-v2)

# Pd sources
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -matomics -mbulk-memory")
include(${CMAKE_CURRENT_SOURCE_DIR}/Pd4Web/libpd.cmake)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Pd4Web/pure-data/src)

# Pd4Web executable
add_executable(pd4web Pd4Web/pd4web.cpp Pd4Web/externals.cpp)
target_include_directories(pd4web PRIVATE Pd4Web/pure-data/src)
target_link_libraries(pd4web PRIVATE
    embind
    libpd
)
set_target_properties(pd4web PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/WebPatch
)
target_link_options(pd4web PRIVATE
    -sMODULARIZE=1
    -sEXPORT_NAME='Pd4WebModule'
    -sINITIAL_MEMORY=128MB
    -sUSE_PTHREADS=1
    -sPTHREAD_POOL_SIZE=4
    -sWASM=1
    -sWASM_WORKERS=1
    -sAUDIO_WORKLET=1
)
include(Pd4Web/Externals/else.cmake)

# Project Externals Libraries
target_link_libraries(pd4web PRIVATE vu_tilde lb args message selector routetype router slice loadbanger wrap2)

# FileSystem for the Patch
set_target_properties(pd4web PROPERTIES LINK_FLAGS "--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/WebPatch/index.pd@/index.pd")
get_target_property(EMCC_LINK_FLAGS pd4web LINK_FLAGS)
set_target_properties(pd4web PROPERTIES LINK_FLAGS "${EMCC_LINK_FLAGS} --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/.tmp@/Libs/")
get_target_property(EMCC_LINK_FLAGS pd4web LINK_FLAGS)
set_target_properties(pd4web PROPERTIES LINK_FLAGS "${EMCC_LINK_FLAGS} --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Extras@/Extras/")
