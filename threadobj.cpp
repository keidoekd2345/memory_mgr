#include "threadobj.h"
CThread_mutex::CThread_mutex()
{

}
CThread_mutex & CThread_mutex::operator=( const CThread_mutex & m )
{
    if (this == &m)
        return *this;

    this->m_mutex = m.m_mutex;
    return *this;
}

CThread_mutex::CThread_mutex(CThread_mutex &m)
{
    m_mutex = m.m_mutex;
}

bool CThread_mutex::init_mutex()
{
    if (0 != pthread_mutex_init(&m_mutex, NULL))
        return false;
    else
        return true;
}

bool CThread_mutex::lock_mutex()
{
    if (pthread_mutex_lock(&this->m_mutex) != 0)
        return false;
    else
        return true;
}

bool CThread_mutex::unlock_mutex()
{
    if (pthread_mutex_unlock(&this->m_mutex) != 0)
        return false;
    else
        return true;
}
CThread_mutex::~CThread_mutex()
{
    pthread_mutex_destroy(&this->m_mutex);
}
