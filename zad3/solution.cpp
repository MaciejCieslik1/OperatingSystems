#include <iostream>
#include <vector>
#include <string>
#include "monitor.h"

// 3 producers, 4 consuments
// 1st producer puts products into 1st queue
// 2nd producer puts products into 4th queue
// 3rd producer puts products into: 1st, 2nd, 3rd, 4th queue alternately 

int const bufferSize = 10;
int const threadsCounts = 7;  // number of threads
int const numberOfProducts = 7;

class Buffer
{
    public:
        std::vector<Semaphore> mutexVec;
        std::vector<Semaphore> fullVec;
        std::vector<Semaphore> stopConsumers;
        std::vector<std::vector<int>> queues;
        int currentQueueProducer;
        std::vector<bool> waitProducers;
        std::vector<bool> waitConsumers;

        Buffer() {
            for (int i = 0; i < 4; i++) {
                mutexVec.push_back(1);
                fullVec.push_back(bufferSize);
                queues.push_back(std::vector<int>());
                stopConsumers.push_back(0);
            }
            waitProducers = {false, false, false};
            waitConsumers = {false, false, false, false};
            currentQueueProducer = -1;
        }

        void put1() {
            // First producer puts product into 1st queue
            fullVec[0].p();
            mutexVec[0].p();
            queues[0].push_back(1);
            printQueueN(0);
            if (canConsumerN(0) && waitConsumers[0])
                stopConsumers[0].v();
            else
                mutexVec[0].v();
        }

        void put2() {
            // Second producer puts product into 4th queue
            fullVec[3].p();
            mutexVec[3].p();
            queues[3].push_back(2);
            std::cout << "\nProducer 2 puts object: ";
            printQueueN(3);
            if (canConsumerN(3) && waitConsumers[3])
                stopConsumers[3].v();
            else
                mutexVec[3].v();
        } 

        void put3() {
            // Third producer puts products into: 1st, 2nd, 3rd, 4th queue alternately
            currentQueueProducer = (currentQueueProducer + 1) % 4;
            fullVec[currentQueueProducer].p();
            mutexVec[currentQueueProducer].p();
            queues[currentQueueProducer].push_back(3);
            std::cout << "\nProducer 3 puts object: ";
            printQueueN(currentQueueProducer);
            if (canConsumerN(currentQueueProducer) && waitConsumers[currentQueueProducer])
                stopConsumers[currentQueueProducer].v();
            else
                mutexVec[currentQueueProducer].v();
        }

        int getN(const int& n) {
            mutexVec[n].p();
            if (!canConsumerN(n))
            {
                waitConsumers[n] = true;
                std::cout << "\nConsumer " << n+1 << " is waiting: ";
                printQueueN(n);
                mutexVec[n].v();
                stopConsumers[n].p();
                waitConsumers[n] = false;
                std::cout << "\nConsumer " << n+1 << " ready: ";
                printQueueN(n);
            }

            int value = queues[n].front();
            std::cout << "\nConsumer " << n+1 << " read: ";
            printQueueN(n);
            queues[n].erase(queues[n].begin());
            fullVec[n].v();
            std::cout << "\nConsumer " << n+1 << " deleted value: ";
            printQueueN(n);
            mutexVec[n].v();
            return value;
        }

        bool canConsumerN(const int& n) {
            if (queues[n].size() > 0) return true;
            return false;
        }

        void printQueueN(const int& n) {
            std::cout << "[";
            for (int value : queues[n]) {
                std:: cout << value << ", ";
            }
            std::cout << "]" << std::endl;
        }
};

Buffer buffer;

void* threadProducer1(void* arg)
{
	for (int i = 0; i < numberOfProducts; ++i)
	{
		buffer.put1();
	}
	std::cout << "### P1 finished ###\n";
	return NULL;
}

void* threadProducer2(void* arg)
{
	for (int i = 0; i < numberOfProducts; ++i)
	{
		buffer.put2();
	}
	std::cout << "### P2 finished ###\n";
	return NULL;
}

void* threadProducer3(void* arg)
{
	for (int i = 0; i < numberOfProducts; ++i)
	{
		buffer.put3();
	}
	std::cout << "### P3 finished ###\n";
	return NULL;
}

void* threadConsumer1(void* arg)
{
	for (int i = 0; i < numberOfProducts; ++i)
	{
		auto value = buffer.getN(0);
	}
	std::cout << "### C1 finished ###\n";
	return NULL;
}

void* threadConsumer2(void* arg)
{
	for (int i = 0; i < 2; ++i)
	{
		auto value = buffer.getN(1);
	}
	std::cout << "### C2 finished ###\n";
	return NULL;
}

void* threadConsumer3(void* arg)
{
	for (int i = 0; i < 2; ++i)
	{
		auto value = buffer.getN(2);
	}
	std::cout << "### C3 finished ###\n";
	return NULL;
}


void* threadConsumer4(void* arg)
{
	for (int i = 0; i < numberOfProducts; ++i)
	{
		auto value = buffer.getN(3);
	}
	std::cout << "### C4 finished ###\n";
	return NULL;
}


int main()
{
	pthread_t tid[threadsCounts];

	pthread_create(&(tid[0]), NULL, threadProducer1, NULL);
	pthread_create(&(tid[1]), NULL, threadProducer2, NULL);
    pthread_create(&(tid[2]), NULL, threadProducer3, NULL);
	pthread_create(&(tid[3]), NULL, threadConsumer1, NULL);
	pthread_create(&(tid[4]), NULL, threadConsumer2, NULL);
    pthread_create(&(tid[5]), NULL, threadConsumer3, NULL);
	pthread_create(&(tid[6]), NULL, threadConsumer4, NULL);

	for (int i = 0; i < threadsCounts; i++)
		pthread_join(tid[i], (void**)NULL);
	return 0;
}


