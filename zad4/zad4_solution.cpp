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
    int queuesSizes[QUEUES_NUMBER];
    int queues[QUEUES_NUMBER][N];
    Condition empty[QUEUES_NUMBER];
    Condition full[QUEUES_NUMBER];

    public:
        SmartBuffer(){
            for (int i = 0; i < QUEUES_NUMBER; i++) {
                queuesSizes[i] = 0;
            }
        }

        void put(int producerIndex) {
            int bufferIndex;
            enter();
            bufferIndex = findBufferIndex(producerIndex);
            if (queuesSizes[bufferIndex] == N) {
                wait(full[bufferIndex]);
            }
            productionHandling(producerIndex, bufferIndex);
            if (queuesSizes[bufferIndex] == 1) {
                signal(empty[bufferIndex]);
            }
            leave();
        }

        void remove(int consumerIndex) {
            enter();
            if (queuesSizes[consumerIndex] == 0) {
                wait(empty[consumerIndex]);
            }
            consume(consumerIndex);
            if (queuesSizes[consumerIndex] == N - 1) {
                signal(full[consumerIndex]);
            }
            leave();
        }

        int findBufferIndex(int producerIndex) {
            int bufferIndex;
            currentQueue = currentQueue % 4;
            switch (producerIndex) {
                case(0):
                    bufferIndex = 0;
                    break;
                case(1):
                    bufferIndex = 3;
                    break;
                case(2):
                    bufferIndex = currentQueue;
                    break;
            }
            if (producerIndex == 2) {
                currentQueue ++;
            }
            return bufferIndex;
        }

        void productionHandling(int producerIndex, int bufferIndex) {
            std::string communicate;
            queues[bufferIndex][queuesSizes[bufferIndex]] = producerIndex + 1;
            queuesSizes[bufferIndex] ++;
            communicate = "Queue " + std::to_string(bufferIndex + 1) + ": Producer " + std::to_string(producerIndex + 1) + " has produced ";
            print(communicate, bufferIndex);
        }

        void consume(int bufferIndex) {
            std::string communicate;
            for (int i = 0; i < queuesSizes[bufferIndex] - 1; i++) {
                queues[bufferIndex][i] = queues[bufferIndex][i + 1];
            }
            queuesSizes[bufferIndex] --;
            communicate = "Queue " + std::to_string(bufferIndex + 1) + ": Consumer " + std::to_string(bufferIndex + 1) + " has consumed ";
            print(communicate, bufferIndex);
        }

        void print(std::string message, int bufferIndex) {
            sem_wait(&printSemaphore);
            std::cout << message;
            std::cout << " [";
            for (int i = 0; i < queuesSizes[bufferIndex]; i++) {
                std::cout << queues[bufferIndex][i] << ", ";
            }
            std::cout << "]" << std::endl;
            sem_post(&printSemaphore);
        }
};

SmartBuffer buffer;

void* threadProducer1(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer.put(0);
	}
	std::cout << "\n### P1 finished ###\n";
	return NULL;
}

void* threadProducer2(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer.put(1);
	}
	std::cout << "\n### P2 finished ###\n";
	return NULL;
}

void* threadProducer3(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer.put(2);
	}
	std::cout << "\n### P3 finished ###\n";
	return NULL;
}

void* threadConsumer1(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer.remove(0);
	}
	std::cout << "\n### C1 finished ###\n";
	return NULL;
}

void* threadConsumer2(void* arg)
{
	for (int i = 0; i < N / 4; ++i)
	{
		buffer.remove(1);
	}
	std::cout << "\n### C2 finished ###\n";
	return NULL;
}

void* threadConsumer3(void* arg)
{
	for (int i = 0; i < N / 4; ++i)
	{
		buffer.remove(2);
	}
	std::cout << "\n### C3 finished ###\n";
	return NULL;
}


void* threadConsumer4(void* arg)
{
	for (int i = 0; i < N; ++i)
	{
		buffer.remove(3);
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
