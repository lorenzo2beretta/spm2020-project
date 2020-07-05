/* 
 * Author: Lorenzo Beretta <lorenzo2beretta@gmail.com> 
 * Date:   June 2020
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include "utimer.hpp"

using namespace ff;

struct task {
    int blk;
    int st;
    int en;
    int parity;
    
    task(int blk, int st, int en, int parity): blk(blk), st(st), en(en) {
	this->parity = parity & 1;
    }
};

struct masterStage: ff_node_t<task> {
    const int n;
    const int nw;
    const int nb;
    std::vector<int> stv, env;
    std::vector<int> npass;
    std::vector<bool> busy;
    int tot_npass = 0;
    
    masterStage(const int n, const int nw): n(n), nw(nw), nb(2 * nw) {
	// define worker bundaries so that they are perfectly balanced
	int delta = n / nb;
	int reminder = n % nb;
	for (int i = 0; i < n; i += delta) {
	    stv.push_back(i);
	    if (reminder-- > 0) i++;
	    env.push_back((i + delta < n) ? (i + delta) : (n - 1));
	}
	npass = std::vector<int>(nb);
	busy = std::vector<bool>(nb);
    };

    task* svc(task* it) {
	if (it == NULL) {
	    for (int i = 0; i < nb; ++i) {
		task* ot = new task(i, stv[i], env[i], 1);
		busy[i] = true;
		ff_send_out(ot);
	    }
	    return GO_ON;
	}

	busy[it->blk] = false;
	npass[it->blk]++;
	delete it;  // prevent memory leaks
	if (++tot_npass == n * nb) {
	    std::cout << "TOT_NPASS == N * NB" << std::endl;
	    return EOS;
	}
	
	for (int blk = 0; blk < nb; ++blk) {
	    bool lcond = (blk == 0) || (npass[blk] <= npass[blk - 1]);
	    bool rcond = (blk == nb - 1) || (npass[blk] <= npass[blk + 1]);

	    if (!busy[blk] && npass[blk] < n && lcond && rcond) {
		busy[blk] = true;
		task* ot = new task(blk, stv[blk], env[blk], npass[blk]);
		ff_send_out(ot);  // implement a custom scheduling?
	    }
	}
	return GO_ON;
    }
};

struct workerStage: ff_node_t<task> {
    std::vector<int> &v;

    workerStage(std::vector<int> &v): v(v) {};
    
    task* svc(task* it) {
	int st = it->st;
	int en = it->en;
	if ((st ^ it->parity) & 1) st++;

	for (; st < en; st += 2)
	    if (v[st + 1] < v[st])
		std::swap(v[st + 1], v[st]);
	return it;
    }
}; 


template<typename T>
void oesort_farm(std::vector<T> &v, int nw) {
    size_t n = v.size();
    std::vector<std::unique_ptr<ff_node>> w;
    for (int i = 0; i < nw; ++i)
	w.push_back(std::make_unique<workerStage>(v));

    ff_Farm<task> farm(std::move(w));
    masterStage master(n, nw);
    farm.add_emitter(master);
    farm.remove_collector();
    farm.wrap_around();
    // farm.set_scheduling_ondemand(3);  // this is a point worth stressing

    if (farm.run_and_wait_end() < 0) {
	error("running farm");
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
	oesort_farm<int>(v, nw);
    }
    
    // check that the algorithm is correct
    if (!std::is_sorted(v.begin(), v.end())) {
	std::cout << "VECTOR IS NOT SORTED!" << std::endl;
	for (auto z : v)
	    std::cout << z << " ";
	std::cout << std::endl;
	return -1;
    }
    return 0;
}
