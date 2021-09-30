cmake_minimum_required(VERSION 3.5)
project(DSCAN)

option(USE_LOG_DEBUG "use log debug" OFF)
if (USE_LOG_DEBUG)
    add_compile_options(-DUSE_LOG=1)
endif ()
set(CMAKE_CXX_STANDARD 11)

# setup pthread environmental variables
find_package(Threads REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH}) # To allow CMake to locate our Find*.cmake files

# define git version
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
add_definitions("-DGIT_SHA1=\"${GIT_SHA1}\"")
message("Git Version: ${GIT_SHA1}")

# find sse/avx2/avx512 cpu-flags
include(FindSSE)
if (AVX2_FOUND)
    message("found AVX2")
    option(AVX2 "enable AVX2" ON)
endif ()

# usgae(knl): cmake .. -DKNL=ON
# usgae(avx2): cmake .. -DKNL=OFF -DAVX2=ON
# usage(sse): cmake .. -DKNL=OFF -DAVX2=OFF
if (KNL)
    message(KNL)
    add_compile_options(-DKNL=1)
    find_package(Memkind)
    link_libraries(${MEMKIND_LIBRARIES})
elseif (AVX2)
    message(AVX2)
else ()
    message(NOT_KNL,NOT_AVX2)
endif ()

include_directories(${CMAKE_SOURCE_DIR})
set(COMMON_UTILS util/log/log.cpp util/md5.cpp util/search/search_util.cpp)
add_library(common-utils ${REORDER_UTILS} ${COMMON_UTILS})
link_libraries(common-utils)

## DSCAN release 1: parallel
set(SOURCE_FILES main.cpp Graph.cpp SetIntersection.cpp Graph.h InputOutput.cpp InputOutput.h
        DisjointSet.cpp DisjointSet.h ThreadPool.h Util.h ThreadSafeDisjointSet.h)
add_executable(DSCANParallel ${SOURCE_FILES})
target_compile_options(DSCANParallel PRIVATE -O3 -g)
target_link_libraries(DSCANParallel ${CMAKE_THREAD_LIBS_INIT})

## DSCAN release 2: parallel with sse4.2
add_executable(DSCANParallelSSE ${SOURCE_FILES})
target_compile_definitions(DSCANParallelSSE PRIVATE ENABLE_SSE=1)
target_compile_options(DSCANParallelSSE PRIVATE -O3 -g -march=native)
target_link_libraries(DSCANParallelSSE ${CMAKE_THREAD_LIBS_INIT})

## DSCAN release 3: parallel with avx2
if (KNL OR AVX2)
    message(REDAY_SIMD)
    add_executable(DSCANParallelAVX2 ${SOURCE_FILES})
    target_compile_definitions(DSCANParallelAVX2 PRIVATE ENABLE_AVX2=1)
    target_compile_options(DSCANParallelAVX2 PRIVATE -O3 -g -march=core-avx2)
    target_link_libraries(DSCANParallelAVX2 ${CMAKE_THREAD_LIBS_INIT})

    add_executable(DSCANParallelAVX2Merge ${SOURCE_FILES})
    target_compile_definitions(DSCANParallelAVX2Merge PRIVATE ENABLE_AVX2_MERGE=1)
    target_compile_options(DSCANParallelAVX2Merge PRIVATE -O3 -g -march=core-avx2)
    target_link_libraries(DSCANParallelAVX2Merge ${CMAKE_THREAD_LIBS_INIT})
endif ()

## DSCAN release 4: parallel with avx512
if (KNL)
    add_executable(DSCANParallelAVX512 ${SOURCE_FILES})
    target_compile_definitions(DSCANParallelAVX512 PRIVATE ENABLE_AVX512=1)
    target_compile_options(DSCANParallelAVX512 PRIVATE -O3 -g -march=native)
    target_link_libraries(DSCANParallelAVX512 ${CMAKE_THREAD_LIBS_INIT})

    # no du/dv for min-max pruning, becomes slower
    add_executable(DSCANParallelAVX512NoDuDv ${SOURCE_FILES})
    target_compile_definitions(DSCANParallelAVX512NoDuDv PRIVATE ENABLE_AVX512_NO_DU_DV=1)
    target_compile_options(DSCANParallelAVX512NoDuDv PRIVATE -O3 -g -march=native)
    target_link_libraries(DSCANParallelAVX512NoDuDv ${CMAKE_THREAD_LIBS_INIT})
endif ()

## DSCAN release 4: parallel with avx512 merge
if (KNL)
    add_executable(DSCANParallelAVX512Merge ${SOURCE_FILES})
    target_compile_definitions(DSCANParallelAVX512Merge PRIVATE ENABLE_AVX512_MERGE=1)
    target_compile_options(DSCANParallelAVX512Merge PRIVATE -O3 -g -march=native)
    target_link_libraries(DSCANParallelAVX512Merge ${CMAKE_THREAD_LIBS_INIT})
endif ()

## utility 1: check binary file, and some playground codes
add_subdirectory(playground)

## utility2: graoph converter edge list to binary files
add_subdirectory(converter)