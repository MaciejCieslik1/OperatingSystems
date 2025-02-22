#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <vector>
#include <string>
#include <semaphore.h>
#include "monitor.h"

#define N 7
#define QUEUES_NUMBER 4

sem_t printSemaphore;
int currentQueue = 0;

class SmartBuffer : Monitor {
    int queueSize;
    int bufferNumber;
    int queue[N];
    Condition empty;
    Condition full;

    public:
        SmartBuffer(int number){
            queueSize = 0;
            bufferNumber = number;
        }

        void put(int element) {
            enter();
            if (queueSize == N) {
                wait(full);
            }
            productionHandling(element);
            if (queueSize == 1) {
                signal(empty);
            }
            leave();
        }

        void remove() {
            enter();
            if (queueSize == 0) {
                wait(empty);
            }
            consume();
            if (queueSize == N - 1) {
                signal(full);
            }
            leave();
        }

        void productionHandling(int element) {
            std::string communicate;
            queue[queueSize] = element;
            queueSize ++;
            communicate = "Queue " + std::to_string(bufferNumber) + ": Producer " + std::to_string(element) + " has produced ";
            print(communicate);
        }

        void consume() {
            std::string communicate;
            for (int i = 0; i < queueSize - 1; i++) {
                queue[i] = queue[i + 1];
            }
            queueSize --;
            communicate = "Queue " + std::to_string(bufferNumber) + ": Consumer " + std::to_string(bufferNumber) + " has consumed ";
            print(communicate);
        }

        void print(std::string message) {
            sem_wait(&printSemaphore);
            std::cout << message;
            std::cout << " [";
            for (int i = 0; i < queueSize; i++) {
                std::cout << queue[i] << ", ";
            }
            std::cout << "]" << std::endl;
            sem_post(&printSemaphore);
        }
};

SmartBuffer buffer1(1);
SmartBuffer buffer2(2);
SmartBuffer buffer3(3);
SmartBuffer buffer4(4);


void* threadProducer1(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer1.put(1);
	}
	std::cout << "\n### P1 finished ###\n";
	return NULL;
}

void* threadProducer2(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer4.put(2);
	}
	std::cout << "\n### P2 finished ###\n";
	return NULL;
}

void* threadProducer3(void* arg)
{
    int currentBufferNumber = 0;
	for (int i = 0; i < N; ++i)
	{
        currentBufferNumber = currentBufferNumber % 4;
        switch (currentBufferNumber) {
            case 0:
                buffer1.put(3);
                break;
            case 1:
                buffer2.put(3);
                break;
            case 2:
                buffer3.put(3);
                break;
            case 3:
                buffer4.put(3);
                break;
        }
        currentBufferNumber ++;
	}
	std::cout << "\n### P3 finished ###\n";
	return NULL;
}

void* threadConsumer1(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer1.remove();
	}
	std::cout << "\n### C1 finished ###\n";
	return NULL;
}

void* threadConsumer2(void* arg)
{
	for (int i = 0; i < N / 4; ++i)
	{
		buffer2.remove();
	}
	std::cout << "\n### C2 finished ###\n";
	return NULL;
}

void* threadConsumer3(void* arg)
{
	for (int i = 0; i < N / 4; ++i)
	{
		buffer3.remove();
	}
	std::cout << "\n### C3 finished ###\n";
	return NULL;
}


void* threadConsumer4(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer4.remove();
	}
	std::cout << "\n### C4 finished ###\n";
	return NULL;
}


int main()
{
    int threadsCounts = 7; 
	pthread_t tid[threadsCounts];
    sem_init(&printSemaphore, 0, 1);

	pthread_create(&(tid[0]), NULL, threadProducer1, NULL);
	pthread_create(&(tid[1]), NULL, threadProducer2, NULL);
    pthread_create(&(tid[2]), NULL, threadProducer3, NULL);
	pthread_create(&(tid[3]), NULL, threadConsumer1, NULL);
	pthread_create(&(tid[4]), NULL, threadConsumer2, NULL);
    pthread_create(&(tid[5]), NULL, threadConsumer3, NULL);
	pthread_create(&(tid[6]), NULL, threadConsumer4, NULL);

	for (int i = 0; i < threadsCounts-1; i++)
		pthread_join(tid[i], (void**)NULL);
	return 0;
}
