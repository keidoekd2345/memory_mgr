#include "hips_memmgr.h"
#include "hips_memmgr.h"
CHips_memmgr g_mem_mgr;
unsigned int hips_i_mem_registe(const char * str_mod_name, unsigned int max_mem_usage)
{
    if(str_mod_name == 0 && max_mem_usage>1024*1024*500)
    {
        return 0;
    }
    return g_mem_mgr.registe(string(str_mod_name), max_mem_usage);
}

unsigned int hips_i_mem_unregiste(unsigned int hhandle)
{
    if(hhandle)
    {
        return !g_mem_mgr.unregiste(hhandle);
    }
    else
    {
        return 0;
    }
}

void * hips_i_mem_malloc(unsigned int mem_handle, size_t size)
{
    if(mem_handle)
    {
        return g_mem_mgr.hips_memmgr_malloc(mem_handle, size);
    }
    else
    {
        return 0;
    }
}

void hips_i_mem_free(void * buffer)
{
    if(buffer)
    {
        g_mem_mgr.hips_memmgr_free(0, buffer);
    }
    return;
}

unsigned int hips_i_mem_query_usage(char *pstr_buf, IN OUT uint32* psize_in_bytes)
{
   return g_mem_mgr.hips_memmgr_query_usage(pstr_buf, psize_in_bytes);
}
