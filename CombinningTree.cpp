#include "CombinningTree.h"
#include <stack>
#include <thread>

#define NUM_THREAD 8 
using namespace std;
CombiningTree ctree(NUM_THREAD);

void Node::precombine_lock()
{
	pthread_mutex_lock(&precombinelock);
}

void Node::precombine_unlock()
{
	pthread_mutex_unlock(&precombinelock);
}

void Node::combine_lock()
{
	pthread_mutex_lock(&combinelock);
}

void Node::combine_unlock()
{
	pthread_mutex_unlock(&precombinelock);
}

void Node::op_lock()
{
	pthread_mutex_lock(&oplock);
}

void Node::op_unlock()
{
	pthread_mutex_unlock(&oplock);
}

void Node::distribute_lock()
{
	pthread_mutex_lock(&distributelock);
}

void Node::distribute_unlock()
{
	pthread_mutex_unlock(&distributelock);
}

bool Node::precombine()
{
	precombine_lock();
	while(locked) {}
	switch(cStatus) {
		case IDLE:
			cStatus = FIRST;
			precombine_unlock();
			return true;
		case FIRST:
			locked = true;
			cStatus = SECOND;
			precombine_unlock();
			return false;
		case ROOT:
			precombine_unlock();
			return false;
		default:
			cout << "unexpected Node state " << endl;
			precombine_unlock();
			return false;
	}
}

int Node::combine(int combined)
{
	combine_lock();	
	while(locked) {}
	locked = true;
	firstValue = combined;
	switch(cStatus) {
		case FIRST:
			combine_unlock();
			return firstValue;
		case SECOND:
			combine_unlock();
			return firstValue + secondValue;
		default:
			cout << "unexpected Node state " << endl;
			combine_unlock();
			return -1;
	}
}

int Node::op(int combined)
{
	op_lock();
	switch(cStatus) {
		case ROOT:{
			int prior = result;
			result += combined;
			op_unlock();
			return prior;
				  }
		case SECOND: {
			secondValue = combined;
			locked = false;
			while(cStatus != RESULT) {}
			locked = false;
			cStatus = IDLE;
			op_unlock();
			return result;
					 }
		default: {
			cout << "unexpected Node state " << endl;
			op_unlock();
			return -1;
				 }
	}
}

void Node::distribute(int prior)
{
	distribute_lock();
	switch(cStatus) {
		case FIRST:
			cStatus = IDLE;
			locked = false;
			break;
		case SECOND:
			result = prior + firstValue;
			cStatus = RESULT;
			break;
		default:
			cout << "unexpected node state " << endl;
			distribute_unlock();
			return (void)0;
	}
	distribute_unlock();
}

int CombiningTree::getAndIncrement(void *id)
{
	stack<Node *> path;
	int my_id = *(int *)id;
	Node *myleaf = leaf[my_id / 2];
	Node *node = myleaf;
	while(node->precombine()) {
		node = node->Parent;
	}
	Node *stop = node;
// combine phase
	node = myleaf;
	int combined = 1;
	while(node != stop) {
		combined = node->combine(combined);
		path.push(node);
		node = node->Parent;
	}
// operation phase
	int prior = stop->op(combined);
//distribution phase
	while(!path.empty()){
		node = path.top();
		path.pop();
		node->distribute(prior);
	}
	return prior;
}

void *GetandInc_wapper(void * ptr)
{
	ctree.getAndIncrement(ptr);
	return NULL;
}

int main()
{
	pthread_t threads[NUM_THREAD];
	for(int t = 0; t < NUM_THREAD; t++) {
		int ret = pthread_create(&threads[t], NULL, GetandInc_wapper, (void *)&t);
		if(ret) {
			cout << "create thread error" << endl;
		}
		pthread_join(threads[t], NULL);
	}
/*	
	for(int k = 0; k < NUM_THREAD; k++) {
		int ret = pthread_join(threads[k], NULL);
	}*/
	return 0;
}

