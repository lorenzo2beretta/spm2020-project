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

// this version exploit a barrier implemented by myself
// using a synchronization mechanism orchestrated by
// the main thread.

template<typename T>
void oesort_pthreads_sync(std::vector<T> &v, int nw) {
    size_t n = v.size();
    int delta = n / nw;
    bool shutdown = false;
    int parity = 0;
    bool sorted = false;

    std::vector<bool> jd(nw, true);
    std::mutex mtx_jd;
    std::condition_variable cv_jd;
    
    int cnt = 0;
    std::mutex mtx_cnt;
    std::condition_variable cv_cnt;
    
    auto body = [&](int tid) {
		    int st = tid * delta;
		    int en = (tid == nw - 1) ? (n - 1) : ((tid + 1) * delta);
		    
		    while (!shutdown) {
			{
			    std::unique_lock<std::mutex> lk(mtx_jd);
			    cv_jd.wait(lk, [&]{return !jd[tid];});
			}
			// do the job
			for (int i = st + ((st & 1) ^ parity); i < en; i += 2) {
			  if (v[i + 1] < v[i]) {
				std::swap(v[i + 1], v[i]);
				sorted = false;  // benign data race
			    }
			}
			// mark the jobe as done in jd[tid]
			mtx_jd.lock();
			jd[tid] = true;
			mtx_jd.unlock();
			// update the done-jobs counter 
			mtx_cnt.lock();
			++cnt;
			mtx_cnt.unlock();
			if (cnt == nw) cv_cnt.notify_one();
		    }
		};

    // spawn threads
    std::vector<std::thread*> tids(nw);
    for (int i = 0; i < nw; ++i)
	tids[i] = new std::thread(body, i);

    // main loop
    while (!sorted) {
  	parity = 1 - parity;
	sorted = true;

	// reset jd vector
	mtx_jd.lock();
	for (int i = 0; i < nw; ++i) jd[i] = false;
	mtx_jd.unlock();
	cv_jd.notify_all();
	// wait for every thread to do its job
     	{
	    std::unique_lock<std::mutex> lk(mtx_cnt);
	    cv_cnt.wait(lk, [&]{return cnt == nw;});
	    cnt = 0;
	}
    }

    // shut down every thread
    shutdown = true;
    mtx_jd.lock();
    for (int i = 0; i < nw; ++i) jd[i] = false;
    mtx_jd.unlock();
    cv_jd.notify_all();
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
	oesort_pthreads_sync<int>(v, nw);
    }

    assert(std::is_sorted(v.begin(), v.end()));
    return 0;
}
