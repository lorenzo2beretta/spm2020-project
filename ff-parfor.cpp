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
#include "utility.hpp"

// this function sorts a vector of T-type elements, where T is
// a type for which the order operator < is defined.
template<typename T>
void oes_parfor(std::vector<T> &v, int nworkers) {
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
    if (argc < 3) {
        std::cerr << "use: " << argv[0]  << " nworkers vector-length\n";
        return -1;
    }
 
    int nw = std::stol(argv[1]);
    int n = std::stol(argv[2]);

    std::vector<int> v(n);
    for (int i = 0; i < n; i++) v[i] = rand() % 100;

    {
	utimer timer("odd-even sort");
	oes_parfor<int>(v, nw);
    }
    
    return 0;
}
