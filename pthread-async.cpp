/* 
 * Author: Lorenzo Beretta <lorenzo2beretta@gmail.com> 
 * Date:   June 2020
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "utimer.hpp"


template<typename T>
void oesort_pthreads_async(std::vector<T> &v, const int nw) {
    const size_t n = v.size();
    const int delta = n / nw;
    bool shutdown = false;

    // ---------------------------- INVARIANT --------------------------
    // sorted[tid] == true iff since the start of the last lineare scan
    // no out-of-order pair where found nor border transposition happened
    // in the tid-th chunk. Therefore sorted[tid] for all tids implies
    // that the array is sorted.
    std::vector<int> sorted(nw);
    std::vector<int> meanwhile(nw);
    // mtx_block[j] protects v[tid * delta], sorted[tid] and meanwhile[tid] 
    std::vector<std::mutex> mtx_block(nw);

    int cnt = 0;
    std::mutex mtx_cnt;
    std::condition_variable cv_cnt;

    auto body = [&](int tid) {
		    int st = tid * delta;
		    int en = (tid == nw - 1) ? (n - 1) : ((tid + 1) * delta);
		    
		    while (!shutdown) {
			bool local_sorted = true;
			mtx_block[tid].lock();
			meanwhile[tid] = false;
			mtx_block[tid].unlock();
			
			for (int j : {0, 1}) {
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

    std::string message = argv[0];
    for (int i = 1; i < argc; ++i)
	message += ' ' + std::string(argv[i]);
    
    {
	utimer timer(message);
	oesort_pthreads_async<int>(v, nw);
    }

    assert(std::is_sorted(v.begin(), v.end()));
    return 0;
}
