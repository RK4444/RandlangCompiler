# Randlang Compiler
## Why?
Compiler fascinate me, since I learned to program. Because of that, I wanted to build a compiler myself one day. When I learned about LLVM, I realised this was the perfect chance and I started according to [this](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) tutorial.
## Basic Structure
The project is ordered approximately like any cpp project is, with a src and a include folder
## Build System
CMake is used as a build system. Note that in order to use the LLVM libraries, I compiled them from source like seen [here](https://llvm.org/docs/CMake.html) and [here](https://youtu.be/KYaojNbujKM?list=PLlONLmJCfHTo9WYfsoQvwjsa5ZB6hjOG5&t=156). Once LLVM is compiled from source, make can be used. From within root folder, best practice is:

```
mkdir build2
make
```

The folder `build2` is for `make` only, since `build` is for `cmake` configuration. The difference here is, that `make` produces a statically linked binary with no debug symbols (`make` didn't let me add for some reason) and the `cmake` version is dynamically linked and contains debug symbols (`cmake` didn't let me link the binary statically for some reason). If you know, what the problem is, feel free to open and issue in the issues page.

```
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
```

## Recommended Development environment
For development, I use vscode with the C++ and CMake extensions installed. If you don't already have, install the `clang` compiler, since it is recommended by the creators of `llvm` and to avoid problems.

## State of the project
It is currently able to emit object files.

## Usage
After build, the main binary called `randlang` can be found in the `build2` directory. It can be used as follows:
```
    randlang <somefile>.rdlg
```

This produces an object file called `output.o`

## Building the example
In the example folder, a piece of randlang code and a `cpp` file can be found. When building the example with `make example`, a binary called `exampleMain` is emitted. The `cpp` code calls the `sum` function defined in randlang. If everything went well, the output `sum of 3.0 and 4.0: 7` should be displayed.
