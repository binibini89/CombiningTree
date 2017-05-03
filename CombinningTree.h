#include <iostream>
#include <mutex>
#include <condition_variable>
#include <stdlib.h>
#include <pthread.h>
#define INFI 256

class Node // Combinning Tree Node Class
{
public:
	enum CStatus {IDLE, FIRST, SECOND, RESULT, ROOT};
	volatile bool locked;
	CStatus cStatus;
	int firstValue, secondValue;
	int result;
	Node *Parent;
	pthread_mutex_t precombinelock;
	pthread_mutex_t combinelock;
	pthread_mutex_t oplock;
	pthread_mutex_t distributelock;

	Node() {
		cStatus = ROOT;
		locked = false;
		Parent = NULL;
		firstValue = 0;
		secondValue = 0;
		result = 0;
		pthread_mutex_init(&precombinelock, NULL);
		pthread_mutex_init(&combinelock, NULL);
		pthread_mutex_init(&oplock, NULL);
		pthread_mutex_init(&distributelock, NULL);
	}

	Node(Node *myParent) {
		Parent = myParent;
		cStatus = IDLE;
		locked = false;
		firstValue = 0;
		secondValue = 0;
		result = 0;
		pthread_mutex_init(&precombinelock, NULL);
		pthread_mutex_init(&combinelock, NULL);
		pthread_mutex_init(&oplock, NULL);
		pthread_mutex_init(&distributelock, NULL);
	}

	bool precombine();
	int combine(int combined);
	int op(int combined);
	void distribute(int prior);
	void precombine_lock();
	void precombine_unlock();
	void combine_lock();
	void combine_unlock();
	void op_lock();
	void op_unlock();
	void distribute_lock();
	void distribute_unlock();
};

class CombiningTree
{
public:
	Node *nodes[INFI]; //head of internal nodes 
	Node *leaf[INFI];  //head of leaf nodes 
public:
	CombiningTree(int width) {
		nodes[0] = new Node();
		for(int i = 1; i < width; i++) {
			nodes[i] = new Node(nodes[(i-1)/2]);
		}
		for (int i = 0; i < (width + 1)/2; i++) {
			leaf[i] = new Node(nodes[width - i -2]);
		}
	}
	int getAndIncrement(void *id);
};

