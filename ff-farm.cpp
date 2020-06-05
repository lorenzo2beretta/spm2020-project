/* 
 * Author: Lorenzo Beretta <lorenzo2beretta@gmail.com> 
 * Date:   June 2020
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include "utimer.hpp"

// Here there are two data races
// On v: it is read-only, no problem though
// On sorted, but it is benign, since any update switch it to true
// Therefore this parallelization is safe
template<typename T>
void oesort_parfor(std::vector<T> &v, int nworkers) {
    size_t n = v.size();
    ff::ParallelFor pf(nworkers); 
    bool sorted = false;
    auto tran = [&](const long i) { if (v[i + 1] < v[i]) {
	    std::swap(v[i + 1], v[i]); sorted = false; } };

    while (!sorted) {
	sorted = true;
	// odd phase
	pf.parallel_for(1, n - 1, 2, tran, nworkers);
	// even phase
	pf.parallel_for(0, n - 1, 2, tran, nworkers);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "use: " << argv[0]  << " nworkers vector-length seed\n";
        return -1;
    }
 
    int nw = std::stol(argv[1]);
    int n = std::stol(argv[2]);
    int seed = std::stol(argv[3]);
    // seed allows to set up fair experiments
    srand(seed);
    std::vector<int> v(n);
    for (auto &z : v) z = rand();
    std::string message = argv[0];
    for (int i = 1; i < argc; ++i)
	message += ' ' + std::string(argv[i]);
    
    {
	utimer timer(message);
	oesort_parfor<int>(v, nw);
    }
    
    assert(std::is_sorted(v.begin(), v.end()));
    return 0;
}
