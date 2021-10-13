/*
Author: Vaibhav Gogte <vgogte@umich.edu>
Aasheesh Kolli <akolli@umich.edu>

ArgoDSM/PThreads version:
Ioannis Anevlavis <ioannis.anevlavis@etascale.com>

This microbenchmark swaps two items in an array.
*/

#include "argo.hpp"
#include "cohort_lock.hpp"

#include <cstdint>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include <string>
#include <fstream>
#include <iostream>

#define NUM_SUB_ITEMS 64
#define NUM_OPS 10000
#define NUM_ROWS 100000
#define NUM_THREADS 1

#define ENABLE_VERIFICATION 1	//Enable/Disable verification functions.

int workrank;
int numtasks;

// Macro for only node0 to do stuff
#define WEXEC(inst) ({ if (workrank == 0) inst; })

void distribute(int& beg,
		int& end,
		const int& loop_size,
		const int& beg_offset,
    		const int& less_equal){
	int chunk = loop_size / numtasks;
	beg = workrank * chunk + ((workrank == 0) ? beg_offset : less_equal);
	end = (workrank != numtasks - 1) ? workrank * chunk + chunk : loop_size;
}

struct Element {
	// An int variable used for verification.
	#if ENABLE_VERIFICATION == 1
		int verification = 0;
	#endif
	int32_t value_[NUM_SUB_ITEMS];
	Element& operator=(Element& other) {
		for (int i= 0; i< NUM_SUB_ITEMS; i++) {
			*(value_ + i) = *(other.value_ + i);
		}
		return *this;
	}
};

struct Datum {
argo::globallock::cohort_lock* lock_;
	// pointer to the hashmap
	Element* elements_;
	// A lock which protects this Datum
	//argo::globallock::cohort_lock* lock_;
};

struct sps {
	Datum* array;
	int num_rows_;
	int num_sub_items_;
};

sps* S;

void datum_init(sps* s) {
	for(int i = 0; i < NUM_ROWS; i++) {
		s->array[i].elements_ = argo::conew_<Element>();
		s->array[i].lock_ = new argo::globallock::cohort_lock(true); //todo change lock declaration to non synch
	}
	WEXEC(std::cout << "Finished allocating elems & locks" << std::endl);

	int beg, end;
	distribute(beg, end, NUM_ROWS, 0, 0);

	std::cout << "rank: "                << workrank
	          << ", initializing from: " << beg
		  << " to: "                 << end
		  << std::endl;

	for(int i = beg; i < end; i++)
		for(int j = 0; j < NUM_SUB_ITEMS; j++)
			s->array[i].elements_->value_[j] = i+j;
	WEXEC(std::cout << "Finished team process initialization" << std::endl);
}

void datum_free(sps* s) {
	for(int i = 0; i < NUM_ROWS; i++) {
		delete s->array[i].lock_;
		argo::codelete_(s->array[i].elements_);
	}
}

void initialize() {
	S = new sps;
	S->num_rows_ = NUM_ROWS;
	S->num_sub_items_ = NUM_SUB_ITEMS;
	S->array = new Datum[NUM_ROWS];
	datum_init(S);
	argo::barrier();
	WEXEC(fprintf(stderr, "Created array at %p\n", (void *)S->array));
}

bool swap(unsigned int index_a, unsigned int index_b) {
	//check if index is out of array
	assert (index_a < NUM_ROWS && index_b < NUM_ROWS);

	//exit if swapping the same index
	if (index_a == index_b)
		return true;

	//enforce index_a < index_b
	if (index_a > index_b) {
		int index_tmp = index_a;
		index_a = index_b;
		index_b = index_tmp;
	}
	//std::cout<<gettid()<<"Entering lock"<<std::endl;
	S->array[index_a].lock_->lock();  //todo examine if locks can be changed.
	S->array[index_b].lock_->lock();

	int* a = &S->array[index_a];
	//std::cout<<gettid()<<"In lock"<<std::endl;
//std::cout<<"sizeof :"<<sizeof(S->array[index_a])<<std::endl;
std::cout<<"sizeof :"<<(&S->array[index_a])<<std::endl;
	std::cout<<"lock is at: "<<&(S->array[index_a].lock_)<<std::endl;
	//std::cout<<&S->array[index_a]<<std::endl;
	std::cout<<"index"<<&(S->array[index_a].elements_)<<std::endl;
	argo::backend::selective_acquire(&(S->array[index_a]).elements_, sizeof(Datum));
	//argo::backend::selective_acquire(&(S->array[index_b]), sizeof(Datum));
	//std::cout<<gettid()<<"Swapping values-0"<<std::endl;
	//argo::backend::selective_acquire(&S->array[index_b].elements_->value_, 256);//sizeof(Element));
	//argo::backend::selective_acquire(&S->array[index_b].elements_, 256);//sizeof(Element));

	//std::cout<<gettid()<<"Swapping values"<<std::endl;
/*
	//swap array values
	Element temp;
	temp = *(S->array[index_a].elements_);
	//std::cout<<gettid()<<"Swapping values1"<<std::endl;
	*(S->array[index_a].elements_) = *(S->array[index_b].elements_);
	//std::cout<<gettid()<<"Swapping values2"<<std::endl;
	*(S->array[index_b].elements_) = temp;

	//std::cout<<gettid()<<"Values swapped"<<std::endl;

	#if ENABLE_VERIFICATION == 1
		S->array[index_a].elements_->verification = S->array[index_a].elements_->verification+ 1;
	#endif
*/
//	argo::backend::selective_release(&S->array[index_a].elements_, 256);//sizeof(Element));
	//argo::backend::selective_release(&S->array[index_b].elements_->value_, 256);//sizeof(Element));

	//argo::backend::selective_release(&(S->array[index_a]), sizeof(Datum));
	//argo::backend::selective_release(&(S->array[index_b]), sizeof(Datum));
std::cout<<gettid()<<"before locks"<<std::endl;
	S->array[index_a].lock_->unlock();
	std::cout<<gettid()<<"between locks"<<std::endl;
	S->array[index_b].lock_->unlock();
std::cout<<gettid()<<"after locks"<<std::endl;
	return true;
}

void* run_stub(void* ptr) {
	for (int i = 0; i < NUM_OPS/(NUM_THREADS*numtasks); ++i) {
		int index_a = rand()%NUM_ROWS;
		int index_b = rand()%NUM_ROWS;
		swap(index_a, index_b);
	}
	return NULL;
}


//For verification purposes #Verification #changed
void verify() {
	long acc = 0;
	for (long i = 0; i < NUM_ROWS; ++i)
		acc += S->array[i].elements_->verification;

	long ops = NUM_THREADS*numtasks;
	ops *= NUM_OPS/(NUM_THREADS*numtasks);

	if (acc == ops)
		std::cout << "VERIFICATION: SUCCESS" << std::endl;
	else
		std::cout << "VERIFICATION: FAILURE" << std::endl;
}
//end of verification function


int main(int argc, char** argv) {
	argo::init(500*1024*1024UL);

	workrank = argo::node_id();
	numtasks = argo::number_of_nodes();

	WEXEC(std::cout << "In main\n" << std::endl);
	struct timeval tv_start;
	struct timeval tv_end;

	std::ofstream fexec;
	WEXEC(fexec.open("exec.csv",std::ios_base::app));
	// This contains the Atlas restart code to find any reusable data
	initialize();
	pthread_t threads[NUM_THREADS];

	gettimeofday(&tv_start, NULL);
	for (int i = 0; i < NUM_THREADS; ++i) {
		pthread_create(&threads[i], NULL, &run_stub, NULL);
	}
	for (int i = 0; i < NUM_THREADS; ++i) {
		pthread_join(threads[i], NULL);
	}
	argo::barrier();
	gettimeofday(&tv_end, NULL);

	WEXEC(fprintf(stderr, "time elapsed %ld us\n",
				tv_end.tv_usec - tv_start.tv_usec +
				(tv_end.tv_sec - tv_start.tv_sec) * 1000000));
	WEXEC(fexec << "SPS" << ", " << std::to_string((tv_end.tv_usec - tv_start.tv_usec) + (tv_end.tv_sec - tv_start.tv_sec) * 1000000) << std::endl);
	WEXEC(fexec.close());
	//for (int i=0; i < NUM_ROWS; i++) { #todo remove
		//std::cout<<*(S->array[99998].elements_)<<std::endl;
		//std::cout<<(sizeof(S->array[0].elements_))<<std::endl;
		//std::cout<<sizeof(S->array[0].verification)<<std::endl;
		//std::cout<<sizeof(Datum().elements_)<<std::endl;
		//std::cout<<sizeof()<<std::endl;
	//}
	#if ENABLE_VERIFICATION == 1
		verify();
	#endif


	datum_free(S);
	delete[] S->array;
	delete S;

	argo::finalize();

	return 0;
}
