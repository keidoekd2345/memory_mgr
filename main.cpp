#include <stdio.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "tinythread.h"
#include "hips_memmgr.h"

using namespace tthread;
bool g_thread_exit = false;
bool g_free = false;
bool g_allocate = false;
CThread_mutex g_outlock;
class CPara
{
    public:
    CPara(CHips_memmgr & mgr, string & name, CThread_mutex & mutex):m_mgr(mgr),m_mod_name(name),m_out_lock(mutex){};

    CPara(const CPara & para ):m_mgr(para.m_mgr),m_out_lock(para.m_out_lock)
    {
        m_mod_name = para.m_mod_name;
    }

    CPara & operator=(const CPara & left)
    {
        if(this == &left)
            return *this;
        m_mgr = left.m_mgr;
        m_mod_name = left.m_mod_name;
    }
    CHips_memmgr & m_mgr;
    string  m_mod_name;
    CThread_mutex & m_out_lock;
};
class CTest_mem_block
{
    public:
    CTest_mem_block(void* p, uint32 size ):m_point(p),m_size(size)
    {

    };
    void *m_point;
    uint32 m_size;
};

void test_memcont(void * para)
{
    CPara * p = (CPara *) para;
    vector<CTest_mem_block> mem_blocks; 
    handle hmem = p->m_mgr.registe(p->m_mod_name, 1024*1024*500);
    if(hmem)
    {
        uint32 size_allocated=0;
        while(!g_allocate)
        {
            this_thread::sleep_for(chrono::seconds(1));
        }
            
        for (int i = 0; i<20;i++)
        {
            uint32 errcode = 0;

            void * pmem = p->m_mgr.hips_memmgr_malloc(hmem, 97, &errcode);
            if(p)
            {
                mem_blocks.push_back(CTest_mem_block(pmem, 97));
                size_allocated+=97;
            }
            else
            {
                p->m_out_lock.lock_mutex();
                cout<< "thread" << this_thread::get_id() <<"mem allocate failed with error code "<< errcode << endl;
                p->m_out_lock.unlock_mutex();
            }
            uint32 msec = rand()%100; 
            this_thread::sleep_for(chrono::milliseconds(msec));
        }
        p->m_out_lock.lock_mutex();
        cout<<"thread "<< this_thread::get_id()<<" "<< hex << size_allocated << "  memory has allocated "<< endl;
        p->m_out_lock.unlock_mutex();


        while(!g_free)
        {
            this_thread::sleep_for(chrono::seconds(1));
        }

        int count = 0;
        uint32 size_freed = 0;
        for ( vector<CTest_mem_block>::iterator it  = mem_blocks.begin(); it != mem_blocks.end(); it++)
        {
            uint32 errcode =0;
            if(count>=10)
            {
                p->m_mgr.hips_memmgr_free(hmem, it->m_point, &errcode);
                size_freed += it->m_size;
                p->m_out_lock.lock_mutex();
                cout<< "thread" << this_thread::get_id() <<"mem free failed with error code "<< errcode << endl;
                p->m_out_lock.unlock_mutex();

            }
            count ++;
        }
        
        p->m_out_lock.lock_mutex();
        cout<<"thread "<< this_thread::get_id()<< " " << hex << size_freed <<" memory has freed "<< endl;
        p->m_out_lock.unlock_mutex();

        while(!g_thread_exit)
            this_thread::sleep_for(chrono::seconds(1));
        
        int errcode = p->m_mgr.unregiste(hmem);

        if(errcode)
        {
            p->m_out_lock.lock_mutex();
            cout<< "thread" << this_thread::get_id() <<"unregiste memory handle failed with error code "<< errcode << endl;
            p->m_out_lock.unlock_mutex();
        }
            

    }

}

int main(int argc, char *argv[])
{
    CHips_memmgr memmgr;
    vector <thread *> vecthread;
    vector <CPara *> vecpara;
    g_outlock.init_mutex();
    for (int i=0 ;i<1 ; i++)
    {
        stringstream modname;
        modname <<"thread"<<i;
        string str = modname.str();
        CPara * ppara = new CPara(memmgr, str, g_outlock);
        vecpara.push_back(ppara);
    }

    for(vector<CPara *>::iterator it = vecpara.begin(); it!= vecpara.end(); it++)
    {
        CPara* ppara = *it;
        thread *p = new thread(test_memcont, ppara);
        vecthread.push_back(p);
    }

    while(1)
    {
        char inputstr[20];
        cin.getline(inputstr,18);
        string strcmd = inputstr;
        if(strcmd=="usage")
        {
            char * p  = 0;
            uint32 size = 0;
            uint32 raw_size = 0;
            raw_size = memmgr.hips_memmgr_query_usage(0, &size);
            if(raw_size > 0)
            {
                p = new char [raw_size -2];
                size = raw_size - 2;
                raw_size = memmgr.hips_memmgr_query_usage(p, &size);
                if(raw_size>0)
                {
                    delete p;
                    raw_size = 1024*1024;
                    p = new char [raw_size];
                    size = raw_size;
                    raw_size = memmgr.hips_memmgr_query_usage(p, &size);
                    if(raw_size)
                    {
                        g_outlock.lock_mutex();
                        cout << "query usage return a wrong value while the input buffer is enought to receive all information" << endl;
                        g_outlock.unlock_mutex();
                    }
                    else
                    {
                        g_outlock.lock_mutex();
                        cout<<p<<endl;
                        g_outlock.unlock_mutex();
                    }
                }
                else
                {
                    g_outlock.lock_mutex();
                    cout << "queyr usage return a wrong value while the input buffer size less than real size" << endl;
                    g_outlock.unlock_mutex();
                }
            }
            else
            {
                g_outlock.lock_mutex();
                cout << "query usage return a wrong value while the input buffer size is zero"<<endl;
                g_outlock.unlock_mutex();
            }
            
        }
        else if(strcmd == "free")
        {
            g_free = true;
            g_outlock.lock_mutex();
            cout<<"free cmd brodcasted" << endl;
            g_outlock.unlock_mutex();

        }
        else if(strcmd == "allocate")
        {
            g_allocate = true;
            g_outlock.lock_mutex();
            cout<< "allocate cmd brodcasted" << endl;
        }
        else if(strcmd == "unregiste")
        {
            g_thread_exit = true;
            g_outlock.lock_mutex();
            cout<<"unregiste cmd brodcasted"<<endl;
            g_outlock.unlock_mutex();
            for(vector<thread *>::iterator it  = vecthread.begin(); 
                it != vecthread.end(); it++)
            {
                (*it)->join();
            }
            g_outlock.lock_mutex();
            cout<<"all thread exited"<<endl;
            g_outlock.unlock_mutex();

        }
        else 
        {
            g_outlock.lock_mutex();
            cout << "unknow cmd, input again"<<endl;
            g_outlock.unlock_mutex();
        }
    }
    

    return 0;
}

