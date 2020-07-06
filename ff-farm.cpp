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
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include "utimer.hpp"

using namespace ff;

// this function transpose an adjacent pair if it is out-of-order
void transpose(std::vector<int> &v, int ind) {
    if (v[ind + 1] < v[ind])
	std::swap(v[ind + 1], v[ind]);
}

struct task {
    int blk;
    int st;
    int en;
    int parity;  
    task(int b, int s, int e, int p): blk(b), st(s), en(e), parity(p) {};
};

struct masterStage: ff_node_t<task> {
    const int n;
    const int nw;
    const int nb;
    std::vector<int> &v;
    std::vector<int> stv, env;
    std::vector<int> npass;
    std::vector<bool> busy;
    int tot_npass = 0;
    
    masterStage(int n, int nw, int nb, std::vector<int> &v): n(n), nw(nw), nb(nb), v(v) {
	int delta = n / nb;
	int reminder = n % nb;
	// define worker bundaries so that they are perfectly balanced
	// (i.e. |env[i] - env[j] - stv[i] + stv[j]| <= 1 for each i and j) 
	for (int i = 0; i < n; i += delta) {
	    stv.push_back(i);
	    if (reminder-- > 0) i++;
	    env.push_back((i + delta < n) ? (i + delta) : (n - 1));
	}
	npass = std::vector<int>(nb);
	busy = std::vector<bool>(nb);
    };

    task* send_task(task* ot) {
	int st = ot->st;
	int en = ot->en;
	int parity = ot->parity;
	int blk = ot->blk;

	// perform boundary transposition first, so
	// it can immediately start adjacent threads
	if (!((st ^ parity) & 1))  // check if st == parity (mod 2)
	    transpose(v, ot->st);
	if ((en ^ parity) & 1)  // check if en - 1 == parity (mod 2)
	    transpose(v, en - 1);

	npass[blk]++;
	// termination case: npass[i] <= n for each i, then tot_npass = n * nb
	// implies that npass[i] == n for each i
	if (++tot_npass == n * nb)
	    return EOS;
	busy[blk] = true;
	ff_send_out(ot);
	return ot;
    }
	
    task* svc(task* it) {
	// first emission of tasks
	if (it == NULL) {
	    for (int i = 0; i < nb; ++i) {
		task* ot = new task(i, stv[i], env[i], 1);		
		send_task(ot);
	    }
	    return GO_ON;
	}

	// the task comes from a worker's feedback loop
	busy[it->blk] = false;
	delete it;  // prevent memory leaks

	// a worker is possibily idle, it is time to emit some tasks
	for (int i = 0; i < nb; ++i) {
	    // ensure that |npass[i] - npass[i + 1]| <= 1 for each i
	    bool lcond = (i == 0) || (npass[i] <= npass[i - 1]);
	    bool rcond = (i == nb - 1) || (npass[i] <= npass[i + 1]);

	    // npass[i] < n ensures that npass[i] <= n for each i and
	    // !busy[i] ensures that each chunk is given to one worker at a time
	    if (!busy[i] && npass[i] < n && lcond && rcond) {
		task* ot = new task(i, stv[i], env[i], npass[i]);
		// termination case
		if(send_task(ot) == EOS)
		    return EOS;
	    }
	}
	return GO_ON;
    }
};

struct workerStage: ff_node_t<task> {
    std::vector<int> &v;

    workerStage(std::vector<int> &v): v(v) {};

# if 0  // this is useful to check that different workers
        // are assigned to different physiscal cores
    int svc_init() {
	std::cout << "Worker " << get_my_id();
	std::cout << " on core " << ff_getMyCpu() <<'\n';
	return 0;
    }
#endif 
    
    task* svc(task* it) {
	int st = it->st;
	int en = it->en;
	// transpose elements in the given chunk having the right parity
	if ((st ^ it->parity) & 1) st++;
	for (; st < en; st += 2)
	    transpose(v, st);
	return it;
    }
}; 


void oesort_farm(std::vector<int> &v, int nw, int nb) {
    size_t n = v.size();

    // create self-destroying workers
    std::vector<std::unique_ptr<ff_node>> w;
    for (int i = 0; i < nw; ++i)
	w.push_back(std::make_unique<workerStage>(v));

    ff_Farm<task> farm(std::move(w));
    masterStage master(n, nw, nb, v);
    farm.add_emitter(master);
    farm.remove_collector();
    farm.wrap_around();
#if 0  // this is useful to compare with various scheduling policies
    farm.set_scheduling_ondemand(3);
#endif
    
    if (farm.run_and_wait_end() < 0) {
	error("running farm");
    }

    farm.ffStats(std::cout);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "use: " << argv[0];
	std::cerr << " nworkers vector-length seed [nblocks]\n";
        return -1;
    }
 
    const int nw = std::stol(argv[1]);
    const int n = std::stol(argv[2]);
    const int seed = std::stol(argv[3]);
    const int nb = (argc == 5) ? std::stol(argv[4]) : 2 * nw;
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
	oesort_farm(v, nw, nb);
    }

    // check that the algorithm is correct
    if (!std::is_sorted(v.begin(), v.end())) {
	std::cout << "VECTOR IS NOT SORTED!" << std::endl;
	return -1;
    }
    return 0;
}
