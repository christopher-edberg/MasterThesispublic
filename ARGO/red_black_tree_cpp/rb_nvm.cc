/*
Author: Vaibhav Gogte <vgogte@umich.edu>
Aasheesh Kolli <akolli@umich.edu>

ArgoDSM/PThreads version:
Ioannis Anevlavis <ioannis.anevlavis@etascale.com>
*/ 

#include "rb.h"

#include <cstdint>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

int workrank;
int numtasks;

// Cohort lock for the whole tree
argo::globallock::cohort_lock* lock_1;

// Macro for only node0 to do stuff
#define WEXEC(inst) ({ if (workrank == 0) inst; })

int* array;
Red_Black_Tree* RB;

void initialize() {
	RB = argo::conew_<Red_Black_Tree>();
}

void* run_stub(void* ptr) {
	for (int i = 0; i < NUM_OPS/(NUM_THREADS*numtasks); ++i) {
		RB->rb_delete_or_insert(NUM_UPDATES_PER_CS);
	}
	return NULL;
}

// check if the two trees contain the same values
void verify(Node* root) {
	unsigned count = 0;
	Node* curr = root;
	while (count < MAX_LEN) {
		int val_1, val_2;
		val_1 = curr->val;
		val_2 = (curr + MAX_LEN)->val;
		assert(val_2 == val_1 && "Two trees do not contain the same values");
		curr++;
		count++;
	}
}

int main(int argc, char** argv) {
	argo::init(500*1024*1024UL);

	workrank = argo::node_id();
	numtasks = argo::number_of_nodes();

	lock_1 = new argo::globallock::cohort_lock();

	WEXEC(std::cout << "In main\n" << std::endl);
	struct timeval tv_start;
	struct timeval tv_end;

	std::ofstream fexec;
	WEXEC(fexec.open("exec.csv",std::ios_base::app));

	// This contains the Atlas restart code to find any reusable data
	initialize();

	array = argo::conew_array<int>(TREE_LENGTH);
	WEXEC(for (int i = 0; i < TREE_LENGTH; ++i) {
			array[i] = i;
	});

	Node* root = argo::conew_array<Node>(2 * MAX_LEN);
	WEXEC(RB->initialize(root, array, TREE_LENGTH));
	argo::barrier();
	WEXEC(std::cout << "Done with RBtree creation" << std::endl);

	pthread_t T[NUM_THREADS];

	gettimeofday(&tv_start, NULL);
	for (int i = 0; i < NUM_THREADS; ++i) {
		pthread_create(&T[i], NULL, &run_stub, NULL);
	}
	for (int i = 0; i < NUM_THREADS; ++i) {
		pthread_join(T[i], NULL);
	}
	argo::barrier();
	gettimeofday(&tv_end, NULL);

	WEXEC(fprintf(stderr, "time elapsed %ld us\n",
				tv_end.tv_usec - tv_start.tv_usec +
				(tv_end.tv_sec - tv_start.tv_sec) * 1000000));
	WEXEC(fexec << "RB" << ", " << std::to_string((tv_end.tv_usec - tv_start.tv_usec) + (tv_end.tv_sec - tv_start.tv_sec) * 1000000) << std::endl);
	WEXEC(fexec.close());

	WEXEC(std::cout << "Done with RBtree" << std::endl);

	delete lock_1;
	argo::codelete_array(array);
	argo::codelete_array(root);
	argo::codelete_(RB);

	WEXEC(std::cout << "Finished!" << std::endl);

	argo::finalize();

	return 0;
}
