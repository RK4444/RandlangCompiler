#include <iostream>

extern "C" {
    double sum(double, double);
    double baz(double);
}

int main() {
    std::cout << "sum of 3.0 and 4.0: " << sum(3.0, 4.0) << std::endl;
    std::cout << "It is either: " << baz(-1) << std::endl;
}