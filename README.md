# Randlang Compiler
## Why?
Compiler fascinate me, since I learned to program. Because of that, I wanted to build a compiler myself one day. When I learned about LLVM, I realised this was the perfect chance and I started according to [this](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) tutorial.
## Basic Structure
The project is ordered approximately like any cpp project is, with a src and a include folder
## Build System
CMake is used as a build system. Note that in order to use the LLVM libraries, I compiled them from source like seen [here](https://llvm.org/docs/CMake.html) and [here](https://youtu.be/KYaojNbujKM?list=PLlONLmJCfHTo9WYfsoQvwjsa5ZB6hjOG5&t=156). Once LLVM is compiled from source, cmake can be used on the CmakeLists.txt file. From within root folder, best practice is:

```
mkdir build && cd build
cmake ..
```

## Recommended Development environment
For development, I use vscode with the C++ and CMake extensions installed. If you don't have the clang compiler installed, you are free to use gcc aswell, although clang is recommended by llvm.
