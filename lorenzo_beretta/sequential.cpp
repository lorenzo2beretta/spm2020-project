/* This is a sequential version of the Odd-Even sorting algorithm
 * as described on https://en.wikipedia.org/wiki/Odd%E2%80%93even_sort.
 *
 * Author: Lorenzo Beretta <lorenzo2beretta@gmail.com> 
 * Date:   June 2020
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include "utimer.hpp"

// This function sorts a vector of T-type elements, where T is
// a type for which the order operator < is defined using odd-even sort.
template<typename T>
void oesort_seq(std::vector<T> &v) {
    size_t n = v.size();
    bool sorted = false;
    while (!sorted) {
	sorted = true;
	// odd phase
	for (int i = 1; i < n - 1; i += 2) {
	    if (v[i + 1] < v[i]) {
		std::swap(v[i + 1], v[i]);
		sorted = false;
	    }
	}
	// even phase
	for (int i = 0; i < n - 1; i += 2) {
	    if (v[i + 1] < v[i]) {
		std::swap(v[i + 1], v[i]);
		sorted = false;
	    }
	}
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "use: " << argv[0]  << " vector-length seed\n";
        return -1;
    }
 
    int n = std::stol(argv[1]);
    int seed = std::stol(argv[2]);
    // seed allows to set up fair experiments
    srand(seed);
    std::vector<int> v(n);
    for (auto &z : v) z = rand();
    std::string message = argv[0];
    for (int i = 1; i < argc; ++i)
	message += ' ' + std::string(argv[i]);
    
    {
	utimer timer(message);
	oesort_seq<int>(v);
    }

    assert(std::is_sorted(v.begin(), v.end()));
    return 0;
}
