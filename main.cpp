#include <stdio.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "tinythread.h"
#include "hips_memmgr.h"
#include "hips_inter_mem.h"
using namespace tthread;
bool g_thread_exit = false;
bool g_free = false;
bool g_allocate = false;
CThread_mutex g_outlock;
class CPara
{
    public:
    CPara(CHips_memmgr & mgr, string & name, CThread_mutex & mutex):m_mgr(mgr),m_mod_name(name),m_out_lock(mutex)
    {
        m_hmemmgr = 0;
    };
    CPara(const CPara & para ):m_mgr(para.m_mgr),m_out_lock(para.m_out_lock)
    {
        m_mod_name = para.m_mod_name;
        m_hmemmgr = para.m_hmemmgr;
    }

    CPara & operator=(const CPara & left)
    {
        if(this == &left)
            return *this;
        m_mgr = left.m_mgr;
        m_mod_name = left.m_mod_name;
        m_hmemmgr = left.m_hmemmgr;
    }
    CHips_memmgr & m_mgr;
    string  m_mod_name;
    handle  m_hmemmgr;
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

#define ALLOCATE_SIZE 23
#define THREAD_NUMBER 10
#define RANDOM_THREAD_NUMBER 10
void out_error(uint32 errorcode)
{
    g_outlock.lock_mutex();
    switch(errorcode)
    {
        case INVALID_PARAMETER: 
        cout<< "thread "<< this_thread::get_id() << " error:"<<"INVALID_PARAMETER"<<endl; 
        break;
        case UNKNOWN_MOD:
         cout<< "thread "<< this_thread::get_id() << " error:"<<"UNKNOWN_MOD" << endl; 
         break;
        case OVER_LIMIT: 
        cout<< "thread "<< this_thread::get_id() << " error:"<<"OVER_LIMIT"<< endl; 
        break;
        case INVALIDE_MEMBLOCK:
        cout<< "thread "<< this_thread::get_id() << " error:"<<"INVALIDE_MEMBLOCK" << endl; 
        break;
        case MEMBLOCK_OVERSTEP:
        cout<< "thread "<< this_thread::get_id() << " error:"<<"MEMBLOCK_OVERSTEP"<< endl;
         break;
        case MEMBLOCK_FATAL_ERROR:
        cout<< "thread "<< this_thread::get_id() << " error:"<<"MEMBLOCK_FATAL_ERROR"<< endl; 
        break;
        case MEMBLOCK_LEAK_FOUND: 
        cout<< "thread "<< this_thread::get_id() << " error:"<<"MEMBLOCK_LEAK_FOUND"<< endl; 
        break;
        default:
        cout<< "unkonw error"<<endl;
        break;
    }
    g_outlock.unlock_mutex();
}
void random_allocate_free_test(void * para)
{
    CPara *p =(CPara *) para;
    while(!g_thread_exit)
    {
        size_t size= rand()%100;
        uint32 errorcode;
        void *pmem = p->m_mgr.hips_memmgr_malloc(p->m_hmemmgr, size, &errorcode);
        if(errorcode)
        {
            out_error(errorcode);
        }
            

        this_thread::sleep_for(chrono::milliseconds(size));

        if(0)//if(size % 10 == 1)
        {
            *((dword *)((byte*)pmem+size))=0;
        }
        p->m_mgr.hips_memmgr_free(p->m_hmemmgr, pmem, &errorcode);
        if(errorcode)
        {
            out_error(errorcode);
        }
    }
    
}
void test_memcont(void * para)
{
    CPara * p = (CPara *) para;
    vector<CTest_mem_block> mem_blocks;
    p->m_out_lock.lock_mutex();
    cout<< "thread:" << this_thread::get_id() <<" started "<< endl;
    p->m_out_lock.unlock_mutex();
    uint32 msec = rand()%100;
    handle hmem = p->m_mgr.registe(p->m_mod_name, 1024*1024*500);

    if(hmem)
    {
        uint32 size_allocated=0;
        while(!g_allocate)
        {
            this_thread::sleep_for(chrono::seconds(1));
        }
            
        if(hmem == 2)
        {
            p->m_mgr.unregiste(hmem);
                
            while(!g_thread_exit)
                this_thread::sleep_for(chrono::seconds(1));

            return; 
        }

        for (int i = 0; i<20;i++)
        {
            uint32 errcode = 0;

            void * pmem = p->m_mgr.hips_memmgr_malloc(hmem, ALLOCATE_SIZE, &errcode);
            if(i == 1)
            {
                *((dword *)(((byte *)pmem)+ALLOCATE_SIZE)) = 0 ;
            }
            if(i == 2)
            {
                *((dword *)(((byte *)pmem)-4)) = 0 ;
            }
            if(p)
            {
                mem_blocks.push_back(CTest_mem_block(pmem, ALLOCATE_SIZE));
                size_allocated+=ALLOCATE_SIZE;
            }
            else
            {
                out_error(errcode);
            }
            msec = rand()%100; 
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
            //if(count>=10)
            {
                p->m_mgr.hips_memmgr_free(hmem, it->m_point, &errcode);
                if(errcode)
                {
                    out_error(errcode);
                }

                size_freed += it->m_size;

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
            out_error(errcode);
        }
    }

}

int main(int argc, char *argv[])
{
    handle g_hrandom_mem = 0;
    CHips_memmgr memmgr;
    vector <thread *> vecthread;
    vector <CPara *> vecpara;
    
    CPara * prandom_para = 0;
    vector <thread *> vec_random_thread;
    g_outlock.init_mutex();

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
                    memset(p, 0, 1024*1024);
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
                        delete p;
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
            
            if(vecpara.size()>0)
            {
                g_outlock.lock_mutex();
                cout<< "free and unregiste first!" << endl;
                g_outlock.unlock_mutex();
                continue;
            }

            for (int i=0 ;i<THREAD_NUMBER ; i++)
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
 
            g_allocate = true;
            g_free = false;
            g_thread_exit = false;
            g_outlock.lock_mutex();
            cout<< "allocate cmd brodcasted" << endl;
            g_outlock.unlock_mutex();
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
                delete (*it);
            }
            vecthread.clear();

            for(vector<CPara*>::iterator it = vecpara.begin(); it!= vecpara.end(); it++)
            {
                delete (*it);
            }
            vecpara.clear();
            
            for(vector <thread *>::iterator it = vec_random_thread.begin();
                it!=vec_random_thread.end(); it++)
            {
                    (*it)->join();
                    delete(*it);
            }
            vec_random_thread.clear();
            memmgr.unregiste(g_hrandom_mem);
            delete prandom_para;

            g_outlock.lock_mutex();
            cout<<"all thread exited"<<endl;
            g_outlock.unlock_mutex();
        }
        else if(strcmd == "random")
        {
            if(vec_random_thread.size()>0)
            {
                g_outlock.lock_mutex();
                cout<<" unregiste first"<<endl;
                g_outlock.unlock_mutex();
                continue;
            }

            vector<thread *> vectrandom;
            uint32 errorcode=0;
            g_hrandom_mem = memmgr.registe("random", 1024*1024*1024, &errorcode);
            
            if(errorcode)
                out_error(errorcode);
            string random_name = "random";
            prandom_para = new CPara(memmgr, random_name, g_outlock);
            prandom_para->m_hmemmgr = g_hrandom_mem;

            //random_allocate_free_test(prandom_para);
            for (int i=0; i<RANDOM_THREAD_NUMBER; i++)
            {
                vec_random_thread.push_back(new thread(random_allocate_free_test, prandom_para));
            } 
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

