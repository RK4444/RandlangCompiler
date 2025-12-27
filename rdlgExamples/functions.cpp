#include <iostream>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT double println() {
    std::cout << "Hello World" << std::endl;
    return 0.0;
}