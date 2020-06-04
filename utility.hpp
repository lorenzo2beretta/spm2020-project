#include <iostream>

// utility function to print a vector
template<typename T>
void print_vector(std::vector<T> &v) {
    std::cout << "[";
    for (auto z : v) std::cout << z << ", ";
    std::cout << "]\n";
}

