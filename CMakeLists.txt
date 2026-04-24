cmake_minimum_required(VERSION 3.20)
project(prime_wasm CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directory — put generated files directly into web/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/web)

add_executable(prime_module
    src/prime_generator.cpp
    src/bindings.cpp
)

target_include_directories(prime_module PRIVATE src)

if(EMSCRIPTEN)
    set_target_properties(prime_module PROPERTIES
        SUFFIX ".js"
        OUTPUT_NAME "prime_module"
    )

    target_compile_options(prime_module PRIVATE
        -O2
        -Wall
        -Wextra
    )

    target_link_options(prime_module PRIVATE
        -lembind
        -sEXPORT_ES6=1
        -sMODULARIZE=1
        -sEXPORT_NAME=CreatePrimeModule
        -sENVIRONMENT=web
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=33554432
        -O2
    )
endif()