#ifndef _threadobj_h
#define _threadobj_h
#include <pthread.h>
class CThread_mutex
{
    public:
    CThread_mutex();
    CThread_mutex & operator= ( const CThread_mutex & m );
    CThread_mutex(CThread_mutex & m);
    bool init_mutex();
    bool lock_mutex();
    bool unlock_mutex();
    ~CThread_mutex();
    private:
    pthread_mutex_t m_mutex;
};
#endif