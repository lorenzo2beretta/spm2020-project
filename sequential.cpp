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
#include "utility.hpp"

// This function sorts a vector of T-type elements, where T is
// a type for which the order operator < is defined using odd-even sort.
template<typename T>
void oes_seq(std::vector<T> &v) {
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
    if (argc < 2) {
        std::cerr << "use: " << argv[0]  << " vector-length\n";
        return -1;
    }
 
    int n = std::stol(argv[1]);

    std::vector<int> v(n);
    for (int i = 0; i < n; i++) v[i] = rand() % 100;

    {
	utimer timer("sequential odd-even sort");
	oes_seq<int>(v);
    }

    assert(std::is_sorted(v.begin(), v.end()));
    return 0;
}
