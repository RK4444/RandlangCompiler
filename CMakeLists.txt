cmake_minimum_required(VERSION 3.10)
project(Compiler)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_COMPILER clang-18)
set(CMAKE_CXX_COMPILER clang++-18)
set(CMAKE_CXX_STANDARD 17)
set(STANDARD_REQUIRED True)
# set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")#just with -static doesn't seem to work
# -static-libgcc -static-libstdc++: use these instead possibly
# set(BUILD_SHARED_LIBS OFF)

# find_package(ZLIB REQUIRED)
# find_package(zstd REQUIRED)
# set(zstd_USE_STATIC_LIBS ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS})

set (srcdir "${PROJECT_SOURCE_DIR}/src")
set (incdir "${PROJECT_SOURCE_DIR}/include")
set(SOURCES ${srcdir}/main.cpp ${srcdir}/Lexer.cpp ${srcdir}/Parser.cpp ${srcdir}/ASTNodes.cpp ${incdir}/Lexer.h ${incdir}/Parser.h ${incdir}/ASTNodes.h ${incdir}/Token.hpp)
add_executable(randlang ${SOURCES})
target_compile_options(randlang PUBLIC ${LLVM_CXXFLAGS})
target_include_directories(randlang PRIVATE ${include})
target_include_directories(randlang PRIVATE ${src})
llvm_map_components_to_libnames(llvm_libs 
    support
    core
    irreader
    native
    ${LLVM_TARGETS_TO_BUILD})

target_link_libraries(randlang ${LLVM_SYSTEM_LIBS} ${llvm_libs})
# target_link_options(randlang PRIVATE -static)