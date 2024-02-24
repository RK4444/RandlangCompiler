cmake_minimum_required(VERSION 3.10)
project(Compiler)
set(CMAKE_CXX_STANDARD 17)
set(STANDARD_REQUIRED True)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set (srcdir "${PROJECT_SOURCE_DIR}/src")
set (incdir "${PROJECT_SOURCE_DIR}/include")
set(SOURCES ${srcdir}/main.cpp ${srcdir}/Lexer.cpp ${srcdir}/Parser.cpp ${srcdir}/ASTNodes.cpp ${incdir}/Lexer.h ${incdir}/Parser.h ${incdir}/ASTNodes.h ${incdir}/Token.hpp)
add_executable(randlang ${SOURCES})
target_include_directories(randlang PRIVATE ${include})
target_include_directories(randlang PRIVATE ${src})
llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(simple-tool ${llvm_libs})