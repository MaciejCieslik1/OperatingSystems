#include <iostream>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <vector>
#include <string>

#define N 7
#define QUEUES_NUMBER 4

void initSemaphores();
void productionHandling(int id, int queueId);
void produce(int id, int queueId);
void print(std::string message, int queueId);
void createProducer(int id);

struct Semaphores {
    sem_t empty[QUEUES_NUMBER];
    sem_t full[QUEUES_NUMBER];
    sem_t mutex[QUEUES_NUMBER];
};

struct Buffer {
    int queuesSizes[QUEUES_NUMBER];
    int queues[QUEUES_NUMBER][N];
};

class SharedMemory {
public:
    static Semaphores* getSemaphores() {
        static int shmid = 0;
        if (shmid == 0) {
            shmid = shmget(IPC_PRIVATE, sizeof(Semaphores), SHM_W | SHM_R);
        }
        return (struct Semaphores*) shmat(shmid, nullptr, 0);
    }

    static Buffer* getQueues() {
        static int shmid = 0;
        if (shmid == 0) {
            shmid = shmget(IPC_PRIVATE, sizeof(Buffer), SHM_W | SHM_R);
        }
        return (struct Buffer*) shmat(shmid, NULL, 0); 
    }
};

Buffer* buffer = SharedMemory::getQueues();
Semaphores* semaphores = SharedMemory::getSemaphores();


void initSemaphores() {
    for (int i=0; i<4; i++) {
        sem_init(&semaphores->empty[i], 1, N);
        sem_init(&semaphores->full[i], 1, 0);
        sem_init(&semaphores->mutex[i], 1, 1);
    }
}


void producer(int id) {
    int currentQueue = 0;
    for (int i = 0; i < 50; i++) {
        currentQueue = currentQueue % 4;
        switch (id) {
            case(0):
                productionHandling(id, 0);
                break;
            case(1):
                productionHandling(id, 3);
                break;
            case(2):
                productionHandling(id, currentQueue);
                break;
        }
        currentQueue++;
    }
}

void productionHandling(int id, int queueId) {
    std::string communicate;
    sem_wait(&semaphores->empty[queueId]);
    sem_wait(&semaphores->mutex[queueId]);
    produce(id, queueId);
    communicate = "Queue " + std::to_string(queueId + 1) + ": Producer " + std::to_string(id + 1) + " has produced ";
    print(communicate, queueId);
    sem_post(&semaphores->mutex[queueId]);
    sem_post(&semaphores->full[queueId]);
}

void produce(int id, int queueId) {
    int index = buffer->queuesSizes[queueId];
    buffer->queues[queueId][index] = id + 1;
    buffer->queuesSizes[queueId]++;
}

void consumer(int id) {
    std::string communicate;
    for (int j = 0; j < 50; j++) {
        sem_wait(&semaphores->full[id]);
        sem_wait(&semaphores->mutex[id]);
        for (int i = 0; i < buffer->queuesSizes[id] - 1; i++) {
            buffer->queues[id][i] = buffer->queues[id][i + 1];
        }
        buffer->queuesSizes[id]--;
        communicate = "Queue " + std::to_string(id + 1) + ": Consumer " + std::to_string(id + 1) + " has consumed ";
        print(communicate, id);
        sem_post(&semaphores->mutex[id]);
        sem_post(&semaphores->empty[id]);
    }
}

void print(std::string message, int queueId) {
    std::cout << message;
    std::cout << " [";
    for (int i = 0; i < buffer->queuesSizes[queueId]; i++) {
        std::cout << buffer->queues[queueId][i] << ", ";
    }
    std::cout << "]" << std::endl;
}

void createProducer(int id) {
    if (fork() == 0) {
        producer(id);
        exit(0);
    }
}


void createConsumer(int id) {
    if (fork() == 0) {
        consumer(id);
        exit(0);
    }
}


int main() {
    initSemaphores();
    createProducer(0);
    createProducer(1);
    createProducer(2);
    createConsumer(0);
    createConsumer(1);
    createConsumer(2);
    createConsumer(3);
    return 0;
}
