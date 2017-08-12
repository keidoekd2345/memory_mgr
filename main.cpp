#include <stdio.h>
#include <vector>
#include "hips_memmgr.h"
#include "tinythread.h"
bool g_thread_exit = false;
bool g_free = false;

class CPara
{
    public:
    CPara(CHips_memmgr & mgr, char * pname)
    {
        m_mgr = mgr;
        m_pname = pname;
    }
    CHips_memmgr & m_mgr;
    char* m_pname
}
void test_memcont(void * para)
{
    CPara * p = (CPara *) para;
    vector<void *> mem_blocks; 
    handle hmem = p->m_mgr.registe(p->m_pname, 1024*1024*500);
    if(hmem)
    {
        for (int i = 0; i<1000;i++)
        {
            int errcode = 0;
            void *p = p->m_mgr.hips_memmgr_malloc(hmem, 97, &errcode);
            if(p)
            {
                mem_blocks.push_back(p);
            }
            else
            {
                cout<< "thread" << this_thread::get_id() <<"mem allocate failed with error code "<< errcode << endl;
            }
            msec = rand()%100; 
            this_thread::sleep_for(chrono::milliseconds(msec));
        }

        count<<"thread "<< this_thread::get_id()<< "memory has allocated "<< endl;

        while(!g_free)
        {
            this_thread::sleep_for(chrono::seconds(1));
        }

        int count = 0;

        for ( vector<void *>::iterator it  = mem_blocks.begin(); it != mem_blocks.end(); it++)
        {
            int errcode =0;

            if(count>=10)
            {
                p->m_mgr.hips_memmgr_free(hmem, *it, &errcode)；
                cout<< "thread" << this_thread::get_id() <<"mem free failed with error code "<< errcode << endl;
            }
            count ++;
        }
        
        count<<"thread "<< this_thread::get_id()<< "memory has freed "<< endl;

        while(!g_thread_exit)
            this_thread::sleep_for(chrono::seconds(1));
        
        int errcode = p->m_mgr.unregiste(hmem);

        if(errcode)
            cout<< "thread" << this_thread::get_id() <<"unregiste memory handle failed with error code "<< errcode << endl;

    }

}

int main(int argc, char *argv[])
{
    return 0;
}

