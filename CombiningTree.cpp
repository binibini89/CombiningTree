#include "CombiningTree.h"
#include <stack>
#include <thread>
#include <unistd.h>

#define NUM_THREAD 3
//#define DEBUG
using namespace std;
CombiningTree ctree(2*NUM_THREAD);

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
	pthread_mutex_unlock(&combinelock);
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

void Node::wait(pthread_cond_t *cond, pthread_mutex_t *cond_mutex)
{
#if 1
	pthread_cond_wait(cond, cond_mutex);
#endif
}

void Node::notify_all(pthread_cond_t *cond)
{
#if 1
	pthread_cond_broadcast(cond);
#endif
}

bool Node::precombine()
{
	precombine_lock();

//	while(cStatus == RESULT) {}
	pthread_mutex_lock(&cond_mutex);
	while(locked) {
		wait(&cond, &cond_mutex);	
	}
	pthread_mutex_unlock(&cond_mutex);
	while(cStatus == RESULT) {}
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
			cout << " precombine unexpected Node state " << cStatus<< "on node " << this->id << endl;
			precombine_unlock();
			return false;
	}
}

int Node::combine(int combined)
{
	combine_lock();	
#ifdef DEBUG
	cout << " combine on node " << this->id << " cStatus is  " << cStatus<< endl;
#endif
	pthread_mutex_lock(&cond_mutex);
	while(locked) {
		wait(&cond, &cond_mutex);	
	}
	locked = true;
	pthread_mutex_unlock(&cond_mutex);
	firstValue = combined;
	switch(cStatus) {
		case FIRST:
			combine_unlock();
			return firstValue;
		case SECOND:
			combine_unlock();
			return firstValue + secondValue;
		default:
			cout << "combine unexpected Node state " << endl;
			combine_unlock();
			return -1;
	}
}

int Node::op(int combined)
{
	op_lock();
#ifdef DEBUG
	cout << " op on node " << this->id << " cStatus is  " << cStatus<< endl;
#endif
	switch(cStatus) {
		case ROOT:{
					  int prior = result;
					  result += combined;
					  op_unlock();
					  return prior;
				  }
		case SECOND: {
						 secondValue = combined;
						 pthread_mutex_lock(&cond_mutex);
						 locked = false;
						 notify_all(&cond);
						 pthread_mutex_unlock(&cond_mutex);
						 pthread_mutex_lock(&cond_mutex_result);
						 while(cStatus != RESULT) {
							 wait(&cond_result, &cond_mutex_result);	
						 }
#ifdef DEBUG
						 cout << " wait due to  result  changed " << endl;
#endif
						 pthread_mutex_unlock(&cond_mutex_result);
						 pthread_mutex_lock(&cond_mutex);
						 locked = false;
						 notify_all(&cond);
						 cStatus = IDLE;
						 pthread_mutex_unlock(&cond_mutex);
						 op_unlock();
						 return result;
					 }
		default: {
					 cout << "op unexpected Node state " << cStatus << "on node " << this->id<< endl;
					 op_unlock();
					 return -1;
				 }
	}
}

void Node::distribute(int prior)
{
	distribute_lock();
#ifdef DEBUG
	cout << " distribute on node " << this->id << " cStatus is  " << cStatus<< endl;
#endif
	switch(cStatus) {
		case FIRST:
			cStatus = IDLE;
			pthread_mutex_lock(&cond_mutex);
			locked = false;
			notify_all(&cond);
			pthread_mutex_unlock(&cond_mutex);
			break;
		case SECOND:
			result = prior + firstValue;
			pthread_mutex_lock(&cond_mutex_result);
			cStatus = RESULT;
			notify_all(&cond_result);
#ifdef DEBUG
			cout <<"change node " << this->id << "state to RESULT" << endl;
#endif
			pthread_mutex_unlock(&cond_mutex_result);
			break;
		default:
			cout << "distribute unexpected node state " << endl;
			break;
	}


	distribute_unlock();
}

int CombiningTree::getAndIncrement(int my_id)
{
	stack<Node *> path;
	//	cout << "my id is " << my_id <<endl;
	Node *myleaf = leaf[my_id / 2];
	Node *node = myleaf;
	while(node->precombine()) {
		node = node->Parent;
		//	cout << my_id <<" pre combile here " << endl;
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

