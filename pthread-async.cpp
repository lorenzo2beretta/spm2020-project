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
#include <condition_variable>
#include "utimer.hpp"


template<typename T>
void oesort_threads(std::vector<T> &v, int nw) {
    size_t n = v.size();
    int delta = n / nw;
    bool shutdown = false;

    std::vector<bool> sorted(nw);
    std::vector<bool> meanwhile(nw);
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
					meanwhile[tid + 1] = true;
					local_sorted = false;
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
					    mtx_block[tid - 1].lock();
					    meanwhile[tid - 1] = true;
					    mtx_block[tid - 1].unlock();
					    local_sorted = false;
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
			if (local_sorted) {
			    mtx_block[tid].lock();
			    if (!meanwhile[tid] && !sorted[tid]) {
				sorted[tid] = true;
				mtx_cnt.lock();
				++cnt;
				mtx_cnt.unlock();
				cv_cnt.notify_one();
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
	oesort_threads<int>(v, nw);
    }

    assert(std::is_sorted(v.begin(), v.end()));
    return 0;
}
