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

class CPara
{
    public:
    CPara(CHips_memmgr & mgr, string & name):m_mgr(mgr),m_mod_name(name){};

    CPara(const CPara & para ):m_mgr(para.m_mgr)
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
};
void test_memcont(void * para)
{
    CPara * p = (CPara *) para;
    vector<void *> mem_blocks; 
    handle hmem = p->m_mgr.registe(p->m_mod_name, 1024*1024*500);
    if(hmem)
    {
        for (int i = 0; i<1000;i++)
        {
            uint32 errcode = 0;
            void * pmem = p->m_mgr.hips_memmgr_malloc(hmem, 97, &errcode);
            if(p)
            {
                mem_blocks.push_back(pmem);
            }
            else
            {
                cout<< "thread" << this_thread::get_id() <<"mem allocate failed with error code "<< errcode << endl;
            }
            uint32 msec = rand()%100; 
            this_thread::sleep_for(chrono::milliseconds(msec));
        }

        cout<<"thread "<< this_thread::get_id()<< "memory has allocated "<< endl;

        while(!g_free)
        {
            this_thread::sleep_for(chrono::seconds(1));
        }

        int count = 0;

        for ( vector<void *>::iterator it  = mem_blocks.begin(); it != mem_blocks.end(); it++)
        {
            uint32 errcode =0;

            if(count>=10)
            {
                p->m_mgr.hips_memmgr_free(hmem, *it, &errcode);
                cout<< "thread" << this_thread::get_id() <<"mem free failed with error code "<< errcode << endl;
            }
            count ++;
        }
        
        cout<<"thread "<< this_thread::get_id()<< "memory has freed "<< endl;

        while(!g_thread_exit)
            this_thread::sleep_for(chrono::seconds(1));
        
        int errcode = p->m_mgr.unregiste(hmem);

        if(errcode)
            cout<< "thread" << this_thread::get_id() <<"unregiste memory handle failed with error code "<< errcode << endl;

    }

}

int main(int argc, char *argv[])
{
    CHips_memmgr memmgr;
    vector <thread *> vecthread;
    vector <CPara *> vecpara;

    for (int i=0 ;i<1 ; i++)
    {
        stringstream modname;
        modname <<"thread"<<i;
        string str = modname.str();
        CPara * ppara = new CPara(memmgr, str);
        vecpara.push_back(ppara);
    }

    for(vector<CPara *>::iterator it = vecpara.begin(); it!= vecpara.end(); it++)
    {
        CPara* ppara = *it;
        thread *p = new thread(test_memcont, ppara);
        vecthread.push_back(p);
    }
    

    return 0;
}

