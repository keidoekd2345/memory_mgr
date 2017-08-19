#include <stdio.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "tinythread.h"
//#include "hips_memmgr.h"
//#include "threadobj.h"
//#include ".../hips_inter_mem.h"
#include "fast_mutex.h"
#include <dlfcn.h> 
using namespace tthread;
using namespace std;
bool g_thread_exit = false;
bool g_free = false;
bool g_allocate = false;

typedef unsigned int (*pfunc_hips_i_mem_registe)(const char * str_mod_name, unsigned int max_mem_usage);
typedef unsigned int (*pfunc_hips_i_mem_unregiste)(unsigned int hhandle);
typedef void * (*pfunc_hips_i_mem_malloc)(unsigned int mem_handle, size_t size);
typedef void (*pfunc_hips_i_mem_free)(void * buffer);
typedef unsigned int (*pfunc_hips_i_mem_query_usage)(char *pstr_buf, unsigned int * psize_in_bytes);

pfunc_hips_i_mem_registe hips_i_mem_registe = 0;
pfunc_hips_i_mem_unregiste hips_i_mem_unregiste = 0;
pfunc_hips_i_mem_malloc hips_i_mem_malloc = 0;
pfunc_hips_i_mem_free hips_i_mem_free = 0;
pfunc_hips_i_mem_query_usage hips_i_mem_query_usage = 0;
void *hso = 0;

bool init_so()
{
    hso = dlopen("../libmemmgr.so",RTLD_NOW);
    if(hso)
    {
       hips_i_mem_registe = (pfunc_hips_i_mem_registe)dlsym(hso, "hips_i_mem_registe");
       hips_i_mem_unregiste = (pfunc_hips_i_mem_unregiste)dlsym(hso, "hips_i_mem_unregiste");
       hips_i_mem_malloc =  (pfunc_hips_i_mem_malloc)dlsym(hso, "hips_i_mem_malloc");
       hips_i_mem_free = (pfunc_hips_i_mem_free)dlsym(hso, "hips_i_mem_free");
       hips_i_mem_query_usage = (pfunc_hips_i_mem_query_usage)dlsym(hso, "hips_i_mem_query_usage");
       if(hips_i_mem_free && hips_i_mem_malloc && hips_i_mem_registe && hips_i_mem_unregiste && hips_i_mem_query_usage)
       {
           return true;
       }
       else
       {
           dlclose(hso);
           return false;
       }
    }
};
bool uninit_so()
{
    if(hso)
        dlclose(hso);
}

fast_mutex g_outlock;
class CPara
{
    public:
    CPara( string & name, fast_mutex & mutex):m_mod_name(name),m_out_lock(mutex)
    {
        m_hmemmgr = 0;
    };
    CPara(const CPara & para ):m_out_lock(para.m_out_lock)
    {
        m_mod_name = para.m_mod_name;
        m_hmemmgr = para.m_hmemmgr;
    }

    CPara & operator=(const CPara & left)
    {
        if(this == &left)
            return *this;
        m_mod_name = left.m_mod_name;
        m_hmemmgr = left.m_hmemmgr;
    }
    string  m_mod_name;
    unsigned int  m_hmemmgr;
    fast_mutex & m_out_lock;
};
class CTest_mem_block
{
    public:
    CTest_mem_block(void* p, unsigned int size ):m_point(p),m_size(size)
    {

    };
    void *m_point;
    unsigned int m_size;
};

#define ALLOCATE_SIZE 23
#define THREAD_NUMBER 10
#define RANDOM_THREAD_NUMBER 10
void out_error(unsigned int errorcode)
{
/*    g_outlock.lock();
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
    g_outlock.unlock();*/
}
void random_allocate_free_test(void * para)
{
    CPara *p =(CPara *) para;
    while(!g_thread_exit)
    {
        size_t size= rand()%100;
        unsigned int errorcode =0;
        void *pmem = hips_i_mem_malloc(p->m_hmemmgr, size);
        if(errorcode)
        {
            out_error(errorcode);
        }
            

        this_thread::sleep_for(chrono::milliseconds(size));

        if(0)//if(size % 10 == 1)
        {
            *((unsigned int *)((unsigned char *)pmem+size))=0;
        }
        hips_i_mem_free( pmem);
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
    p->m_out_lock.lock();
    cout<< "thread:" << this_thread::get_id() <<" started "<< endl;
    p->m_out_lock.unlock();
    unsigned int  msec = rand()%100;
    unsigned int hmem = hips_i_mem_registe(p->m_mod_name.c_str(), 1024*1024*500);

    if(hmem)
    {
        unsigned int size_allocated=0;
        while(!g_allocate)
        {
            this_thread::sleep_for(chrono::seconds(1));
        }
        for (int i = 0; i<20;i++)
        {
            unsigned int errcode = 0;

            void * pmem  = hips_i_mem_malloc(hmem, ALLOCATE_SIZE);
            if(i == 1)
            {
                *((unsigned int *)(((unsigned char *)pmem)+ALLOCATE_SIZE)) = 0 ;
            }
            if(i == 2)
            {
                *((unsigned int *)(((unsigned char *)pmem)-4)) = 0 ;
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
        p->m_out_lock.lock();
        cout<<"thread "<< this_thread::get_id()<<" "<< hex << size_allocated << "  memory has allocated "<< endl;
        p->m_out_lock.unlock();


        while(!g_free)
        {
            this_thread::sleep_for(chrono::seconds(1));
        }

        int count = 0;
        unsigned int size_freed = 0;
        for ( vector<CTest_mem_block>::iterator it  = mem_blocks.begin(); it != mem_blocks.end(); it++)
        {
            unsigned int errcode =0;
            //if(count>=10)
            {
                hips_i_mem_free(it->m_point);
                if(errcode)
                {
                    out_error(errcode);
                }

                size_freed += it->m_size;

            }
            count ++;
        }
        
        p->m_out_lock.lock();
        cout<<"thread "<< this_thread::get_id()<< " " << hex << size_freed <<" memory has freed "<< endl;
        p->m_out_lock.unlock();

        while(!g_thread_exit)
            this_thread::sleep_for(chrono::seconds(1));
        
        int errcode = hips_i_mem_unregiste(hmem);

        if(errcode)
        {
            out_error(errcode);
        }
    }

    p->m_out_lock.lock();
    cout<<"thread "<< this_thread::get_id()<< " memory has freed "<< endl;
    p->m_out_lock.unlock();
}

int main(int argc, char *argv[])
{
    unsigned int g_hrandom_mem = 0;
    //CHips_memmgr memmgr;
    vector <thread *> vecthread;
    vector <CPara *> vecpara;
    
    CPara * prandom_para = 0;
    vector <thread *> vec_random_thread;
    init_so();
    while(1)
    {
        char inputstr[20];
        cin.getline(inputstr,18);
        string strcmd = inputstr;
        if(strcmd=="usage")
        {
            char * p  = 0;
            unsigned int size = 0;
            unsigned int raw_size = 0;
            raw_size = hips_i_mem_query_usage(0, &size);
            if(raw_size > 0)
            {
                p = new char [raw_size -2];
                size = raw_size - 2;
                raw_size = hips_i_mem_query_usage(p, &size);
                if(raw_size>0)
                {
                    delete p;
                    raw_size = 1024*1024;
                    p = new char [raw_size];
                    memset(p, 0, 1024*1024);
                    size = raw_size;
                    raw_size = hips_i_mem_query_usage(p, &size);
                    if(raw_size)
                    {
                        g_outlock.lock();
                        cout << "query usage return a wrong value while the input buffer is enought to receive all information" << endl;
                        g_outlock.unlock();
                    }
                    else
                    {
                        g_outlock.lock();
                        cout<<p<<endl;
                        g_outlock.unlock();
                        delete p;
                    }
                }
                else
                {
                    g_outlock.lock();
                    cout << "queyr usage return a wrong value while the input buffer size less than real size" << endl;
                    g_outlock.unlock();
                }
            }
            else
            {
                g_outlock.lock();
                cout << "query usage return a wrong value while the input buffer size is zero"<<endl;
                g_outlock.unlock();
            }
            
        }
        else if(strcmd == "free")
        {
            g_free = true;
            g_outlock.lock();
            cout<<"free cmd brodcasted" << endl;
            g_outlock.unlock();

        }
        else if(strcmd == "allocate")
        {
            
            if(vecpara.size()>0)
            {
                g_outlock.lock();
                cout<< "free and unregiste first!" << endl;
                g_outlock.unlock();
                continue;
            }

            for (int i=0 ;i<THREAD_NUMBER ; i++)
            {
                stringstream modname;
                modname <<"thread"<<i;
                string str = modname.str();
                CPara * ppara = new CPara(str, g_outlock);
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
            g_outlock.lock();
            cout<< "allocate cmd brodcasted" << endl;
            g_outlock.unlock();
        }
        else if(strcmd == "unregiste")
        {
            g_thread_exit = true;
            g_outlock.lock();
            cout<<"unregiste cmd brodcasted"<<endl;
            g_outlock.unlock();
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
            hips_i_mem_unregiste(g_hrandom_mem);
            delete prandom_para;

            g_outlock.lock();
            cout<<"all thread exited"<<endl;
            g_outlock.unlock();
        }
        else if(strcmd == "random")
        {
            if(vec_random_thread.size()>0)
            {
                g_outlock.lock();
                cout<<" unregiste first"<<endl;
                g_outlock.unlock();
                continue;
            }

            vector<thread *> vectrandom;
            unsigned int errorcode=0;
            g_hrandom_mem = hips_i_mem_registe("random", 1024*1024*1024);
            
            if(errorcode)
                out_error(errorcode);
            string random_name = "random";
            prandom_para = new CPara(random_name, g_outlock);
            prandom_para->m_hmemmgr = g_hrandom_mem;

            //random_allocate_free_test(prandom_para);
            for (int i=0; i<RANDOM_THREAD_NUMBER; i++)
            {
                vec_random_thread.push_back(new thread(random_allocate_free_test, prandom_para));
            } 
        }
        else 
        {
            g_outlock.lock();
            cout << "unknow cmd, input again"<<endl;
            g_outlock.unlock();
        }
    }
    

    return 0;
}

