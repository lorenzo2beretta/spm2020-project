/* 
 * Author: Lorenzo Beretta <lorenzo2beretta@gmail.com> 
 * Date:   June 2020
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <omp.h>
#include "utimer.hpp"

template<typename T>
void oesort_omp(std::vector<T> &v, int nworkers) {
    size_t n = v.size();
    bool sorted = false;
    
#pragma omp parallel num_threads(nworkers)
    while (true) {
	if (sorted) break;
	
#pragma omp critical
	sorted = true;

#pragma omp barrier
	
#pragma omp for  // odd phase
	for (int i = 1; i < n - 1; ++i) {
	    if (v[i + 1] < v[i]) {
		std::swap(v[i + 1], v[i]);
#pragma omp critical
		sorted = false;
	    }
	}
	
#pragma omp for  // even phase
	for (int i = 0; i < n - 1; i += 2) {
	    if (v[i + 1] < v[i]) {
		std::swap(v[i + 1], v[i]);
#pragma omp critical
		sorted = false;
	    }
	}
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "use: " << argv[0]  << " nworkers vector-length seed\n";
        return -1;
    }
 
    int nw = std::stol(argv[1]);
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
	oesort_omp<int>(v, nw);
    }

    assert(std::is_sorted(v.begin(), v.end()));
    return 0;
}
