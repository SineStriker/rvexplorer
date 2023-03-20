cmake_minimum_required(VERSION 3.16)

project(rvexplore)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_options("-O3")
    endif()
endif()

set(RV32IMA_BINARY_FILE "/home/riscv/VSCodeProjects/mini-rv32ima/baremetal2/baremetal.bin")

add_subdirectory(src)