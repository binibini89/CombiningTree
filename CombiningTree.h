#include <iostream>
#include <mutex>
#include <condition_variable>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>

#define INFI 256
struct Args
{
	int id;
};

enum CStatus {IDLE, FIRST, SECOND, RESULT, ROOT};

class Node // Combinning Tree Node Class
{
public:

	bool locked;
	CStatus cStatus;
	int firstValue, secondValue;
	int result;
	Node *Parent;
	pthread_mutex_t nodelock;
	pthread_cond_t cond;
	pthread_cond_t cond_result;

	int id;

	Node() {
		cStatus = ROOT;
		locked = false;
		Parent = NULL;
		firstValue = 0;
		secondValue = 0;
		result = 0;
		pthread_mutex_init(&nodelock, NULL);
		pthread_cond_init(&cond, NULL);
		pthread_cond_init(&cond_result, NULL);
	}

	Node(Node *myParent) {
		Parent = myParent;
		cStatus = IDLE;
		locked = false;
		firstValue = 0;
		secondValue = 0;
		result = 0;
		pthread_mutex_init(&nodelock, NULL);
		pthread_cond_init(&cond, NULL);
		pthread_cond_init(&cond_result, NULL);
	}

	bool precombine();
	int combine(int combined);
	int op(int combined);
	void distribute(int prior);
	void wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
	void notify_all(pthread_cond_t *cond);
	void node_lock();
	void node_unlock();
};

class CombiningTree
{
public:
	Node *nodes[INFI]; //head of internal nodes 
	Node *leaf[INFI];  //head of leaf nodes 
public:
	CombiningTree(int width) {
		nodes[0] = new Node();
		nodes[0]->id = 0;
		for(int i = 1; i < width-1 ; i++) {
			nodes[i] = new Node(nodes[(i-1)/2]);
			nodes[i]->id = i;
		}
		for (int i = 0; i < (width + 1)/2; i++) {
			leaf[(width+1)/2 - i -1] =nodes[width - i -2];
		}
	}
	int getAndIncrement(int id);
};

