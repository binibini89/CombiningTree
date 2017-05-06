#include "CombiningTree.h"
#include <stack>
#include <thread>
#include <unistd.h>
#define NUM_THREAD 8
//#define DEBUG
using namespace std;
CombiningTree ctree(2*NUM_THREAD);

void Node::node_lock()
{
	pthread_mutex_lock(&nodelock);
}

void Node::node_unlock()
{
	pthread_mutex_unlock(&nodelock);
}

void Node::wait(pthread_cond_t *cond, pthread_mutex_t *cond_mutex)
{
	pthread_cond_wait(cond, cond_mutex);
}

void Node::notify_all(pthread_cond_t *cond)
{
	pthread_cond_broadcast(cond);
}

bool Node::precombine()
{
	node_lock();
	while(locked) {
		wait(&cond, &nodelock);	
	}
	while(cStatus == RESULT) {}
	switch(cStatus) {
		case IDLE:
			cStatus = FIRST;
			node_unlock();
			return true;
		case FIRST:
			locked = true;
			cStatus = SECOND;
			node_unlock();
			return false;
		case ROOT:
			node_unlock();
			return false;
		default:
			cout << " precombine unexpected Node state " << cStatus<< "on node " << this->id << endl;
			node_unlock();
			return false;
	}
}

int Node::combine(int combined)
{
	node_lock();	
	while(locked) {
		wait(&cond, &nodelock);	
	}
	locked = true;
	firstValue = combined;
	switch(cStatus) {
		case FIRST:
			node_unlock();
			return firstValue;
		case SECOND:
			node_unlock();
			return firstValue + secondValue;
		default:
			cout << "combine unexpected Node state " << endl;
			node_unlock();
			return -1;
	}
}

int Node::op(int combined)
{
	node_lock();
	switch(cStatus) {
		case ROOT:{
					  int prior = result;
					  result += combined;
					  node_unlock();
					  return prior;
				  }
		case SECOND: {
						 secondValue = combined;
						 locked = false;
						 notify_all(&cond); 
						 while(cStatus != RESULT) {
							 wait(&cond_result, &nodelock);	
						 }
						 cStatus = IDLE;
						 locked = false;
						 notify_all(&cond);
						 node_unlock();
						 return result;
					 }
		default: {
					 cout << "op unexpected Node state " << cStatus << " on node " << this->id<< endl;
					 node_unlock();
					 return -1;
				 }
	}
}

void Node::distribute(int prior)
{
	node_lock();
	switch(cStatus) {
		case FIRST:
			cStatus = IDLE;
			locked = false;
			notify_all(&cond);
			break;
		case SECOND:
			result = prior + firstValue;
			cStatus = RESULT;
			notify_all(&cond_result);
			break;
		default:
			cout << "distribute unexpected node state " << endl;
			break;
	}


	node_unlock();
}

int CombiningTree::getAndIncrement(int my_id)
{
	stack<Node *> path;
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

void sleep(int intense)
{
	for(int i = 0; i < intense; i++){}
}

void *GetandInc_wapper(void * ptr)
{
	struct Args *arg = (struct Args *)ptr;

	int ret;
	for(int i = 0; i < 1000000;i++) {
		//	cout << "thread " << arg->id << " i is " << i << endl;
		ret = ctree.getAndIncrement(arg->id);
		//	sleep(6000);
	}
	cout << "thread " << arg->id << " is out " << endl;
	return NULL;
}

int main()
{
	pthread_t threads[NUM_THREAD];
	struct Args arg[NUM_THREAD];
	for (int i = 0; i < NUM_THREAD; i++) {
		arg[i].id = i;
	}
	for(int t = 0; t < NUM_THREAD; t++) {
		int ret = pthread_create(&threads[t], NULL, &GetandInc_wapper, (void *)&arg[t]);
		if(ret) {
			cout << "create thread error" << endl;
		}
	}

	for(int k = 0; k < NUM_THREAD; k++) {
		int ret = pthread_join(threads[k], NULL);
	}

	cout << "result is " << ctree.nodes[0]->result << endl;
	return 0;
}

