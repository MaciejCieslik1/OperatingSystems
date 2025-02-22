#ifndef __monitor_h
#define __monitor_h

#include <stdio.h> 
#include <stdlib.h> 
#include <iostream>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <string.h> 
#include <errno.h> 
#include <fcntl.h> 
#include <pthread.h> 
#include <unistd.h>
#include <semaphore.h>

class Semaphore
{
    public:

    Semaphore( int value )
    {
        if( sem_init( & sem, 0, value ) != 0 )
        throw "sem_init: failed";
    }

    ~Semaphore()
    { 
        sem_destroy( & sem ); 
    }

    void p()
    {
        if( sem_wait( & sem ) != 0 )
        throw "sem_wait: failed";
    }

    void v()
    {
        if( sem_post( & sem ) != 0 )
        throw "sem_post: failed";
    }

    int getSem(void) 
    {
        int value;
        if (sem_getvalue(&sem, &value) != 0)
            throw "sem_getvalue: failed";
        return value;
    }

    void setSem(const int& newSem) 
    {
        sem_destroy(&sem);
        if (sem_init(&sem, 0, newSem) != 0)
            throw "sem_init: failed";
    }
    

    private:
        sem_t sem;
};

#endif
