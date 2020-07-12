/* 
 * Author: Lorenzo Beretta <lorenzo2beretta@gmail.com> 
 * Date:   June 2020
 */
/* 
REMARK: All this code is written to sort a vector of integer, however it can 
be easily generalized using templates, in fact we only need that a binary 
operator < implementing a total order relation is implemented over vector 
elements' type.
*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "utimer.hpp"

void oesort_pthreads_async(std::vector<int> &v, const int nw) {
    const size_t n = v.size();
    const int delta = n / nw;
    int reminder = n % nw;
    bool shutdown = false;

    // ---------------------------- INVARIANT --------------------------
    // sorted[tid] == true iff since the start of the last linear scan
    // no out-of-order pair where found nor border transposition happened
    // in the tid-th chunk. Therefore sorted[tid] for all tids implies
    // that the array is sorted.
    std::vector<int> sorted(nw);
    // meanwhile guaranteed the following invariant useful to enforce the one above:
    // meanwhile[tid] == true iff one of the threads (tid - 1) or (tid + 1)
    // performed a border transposition since the start of the current tid main loop
    std::vector<int> meanwhile(nw);
    
    // mtx_block[tid] protects v[stv[tid]], sorted[tid] and meanwhile[tid] 
    std::vector<std::mutex> mtx_block(nw);
    // cnt counts how many chunks have sorted[] set to true
    int cnt = 0;
    std::mutex mtx_cnt;
    std::condition_variable cv_cnt;

    // define worker bundaries so that they are perfectly balanced
    // (i.e. |env[i] - env[j] - stv[i] + stv[j]| <= 1 for each i and j) 
    std::vector<int> stv, env;
    for (int i = 0; i < n; i += delta) {
	stv.push_back(i);
	if (reminder-- > 0) i++;
	env.push_back((i + delta < n) ? (i + delta) : (n - 1));
    }

    // this is the worker's body
    auto body = [&](int tid) {
		    int st = stv[tid];
		    int en = env[tid];

		    // main loop
		    while (!shutdown) {
			// local_sorted == false iff we found out-of-order pairs
			bool local_sorted = true;
			mtx_block[tid].lock();
			meanwhile[tid] = false;
			mtx_block[tid].unlock();
			
			for (int j : {0, 1}) { // even and odd iteration
			    for (int i = st + j; i < en; i += 2) {
				// right border case
				if (i == en - 1 && en != n - 1) {
				    mtx_block[tid + 1].lock();
				    if (v[en] < v[en - 1]) {
					std::swap(v[en], v[en - 1]);
					local_sorted = false;
					meanwhile[tid + 1] = true;
					if (sorted[tid + 1]) {
					    sorted[tid + 1] = false;
					    mtx_cnt.lock();
					    --cnt;
					    mtx_cnt.unlock();
					}
				    }
				    mtx_block[tid + 1].unlock();
				}
				else {
				    // left border case
				    if (i == st && st != 0) {
					mtx_block[tid].lock();
					if (v[st + 1] < v[st]) {
					    std::swap(v[st + 1], v[st]);
					    local_sorted = false;
					    mtx_block[tid - 1].lock();
					    meanwhile[tid - 1] = true;
					    if (sorted[tid - 1]) {
						sorted[tid - 1] = false;
						mtx_cnt.lock();
						--cnt;
						mtx_cnt.unlock();
					    }
					    mtx_block[tid - 1].unlock();
					}
					mtx_block[tid].unlock();
				    }
				    // internal case
				    else {
					if (v[i + 1] < v[i]) {
					    std::swap(v[i + 1], v[i]);
					    local_sorted = false;
					}
				    }
				}
			    }
			}
			// set sorted[tid]
			if (local_sorted) {
			    mtx_block[tid].lock();
			    if (!meanwhile[tid] && !sorted[tid]) {
				sorted[tid] = true;
				mtx_cnt.lock();
				++cnt;
				mtx_cnt.unlock();
				// notify main thread that vector is sorted
				if (cnt == nw) cv_cnt.notify_one();
			    }
			    mtx_block[tid].unlock();
			}
		    }
		};

    // spawn threads
    std::vector<std::thread*> tids(nw);
    for (int i = 0; i < nw; ++i)
	tids[i] = new std::thread(body, i);
    {
	std::unique_lock<std::mutex> lk(mtx_cnt);
	cv_cnt.wait(lk, [&]{return cnt == nw;});
    }
    // shut down every thread
    shutdown = true;  // benign data race
    // join threads and destruct thread objects
    for (int i = 0; i < nw; ++i) {
	tids[i]->join();
	delete tids[i];
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "use: " << argv[0]  << " nworkers vector-length seed\n";
        return -1;
    }
 
    const int nw = std::stol(argv[1]);
    const int n = std::stol(argv[2]);
    const int seed = std::stol(argv[3]);
    // seed allows to set up fair experiments
    srand(seed);
    std::vector<int> v(n);
    for (auto &z : v) z = rand();
    // writing a message, a useful log for managing experiments
    std::string message = argv[0];
    for (int i = 1; i < argc; ++i)
	message += ' ' + std::string(argv[i]);
    
    {
	utimer timer(message);
	oesort_pthreads_async(v, nw);
    }

    // check that the algorithm is correct
    if (!std::is_sorted(v.begin(), v.end())) {
	std::cout << "VECTOR IS NOT SORTED!" << std::endl;
	return -1;
    }
    return 0;
}
