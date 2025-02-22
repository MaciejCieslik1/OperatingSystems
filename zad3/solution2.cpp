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

void initSemaphores();
void productionHandling(int id, int queueId);
void produce(int id, int queueId);
void print(std::string message, int queueId);
void createProducer(int id);

struct Semaphores {
    std::vector<sem_t> empty;
    std::vector<sem_t> full;
    std::vector<sem_t> mutex;
};

class SharedMemory {
public:
    static Semaphores* getSemaphores() {
        static int shmid = 0;
        if (shmid == 0) {
            shmid = shmget(IPC_PRIVATE, sizeof(Semaphores), SHM_W | SHM_R);
            if (shmid < 0) {
                std::cerr << "shmget error\n";
                exit(1);
            }
        }
        void* data = shmat(shmid, nullptr, 0);
        return reinterpret_cast<Semaphores*>(data);
    }

    static std::vector<std::vector<int>>* getBuffer() {
        static int shmid = 0;
        if (shmid == 0) {
            shmid = shmget(IPC_PRIVATE, sizeof(std::vector<int>) * 4, SHM_W | SHM_R); // 4 to liczba wektorów
            if (shmid < 0) {
                std::cerr << "shmget error\n";
                exit(1);
            }
        }
        void* data = shmat(shmid, nullptr, 0);
        return new(data) std::vector<std::vector<int>>(4);
    }
};

std::vector<std::vector<int>>* buffer = SharedMemory::getBuffer();
Semaphores* semaphores = SharedMemory::getSemaphores();


void initSemaphores() {
    for (int i=0; i<4; i++) {
        semaphores->empty.push_back(sem_t());
        semaphores->full.push_back(sem_t());  
        semaphores->mutex.push_back(sem_t()); 
        sem_init(&semaphores->empty[i], 1, N);
        sem_init(&semaphores->full[i], 1, 0);
        sem_init(&semaphores->mutex[i], 1, 1);
    }
}


void producer(int id) {
    int currentQueue = 0;
    std::string communicate;
    while (1) {
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
                std::cout << "kolejka" << currentQueue << std::endl;
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
    sem_post(&semaphores->mutex[queueId]);
    sem_post(&semaphores->full[queueId]);
    communicate = "Producer " + std::to_string(id + 1) + " has producted ";
    print(communicate, queueId);
}

void produce(int id, int queueId) {
    buffer->at(queueId).push_back(id + 1);
}

void consumer(int id) {
    std::string communicate;
    while (1) {
        print("Consument is ready", id);
        sem_wait(&semaphores->full[id]);
        sem_wait(&semaphores->mutex[id]);
        buffer->at(id).erase( buffer->at(id).begin());
        sem_post(&semaphores->mutex[id]);
        sem_post(&semaphores->empty[id]);
        communicate = "Consumer " + std::to_string(id + 1) + " has consumed ";
        print(communicate, id);
    }
}

void print(std::string message, int queueId) {
    std::cout << message;
    std::cout << " [";
    for (int value : buffer->at(queueId)){
        std:: cout << value << ", ";
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

    while (1) {}

    return 0;
}
