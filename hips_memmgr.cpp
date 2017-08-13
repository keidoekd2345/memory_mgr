#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <string.h>
#include "hips_memmgr.h"
#include "hips_inter_mem.h"
using namespace std;
CHips_memmgr g_memmgr;
CHips_memmgr::CHips_memmgr()
{
    m_module_mutex.init_mutex();
}
int CHips_memmgr::unregiste(handle mem_handle)
{
    mod_map::iterator mod_it = this->m_modules.find( mem_handle );
    if(mod_it == this->m_modules.end())
    {
        
        return INVALID_PARAMETER;
    }

    this->hips_memmgr_mod_lock();
    CHips_memmgr_mod & mod = mod_it->second;

    if(mod.m_mem_block_lst.size()>0)
    {
        for (
                memblock_lst::iterator block_it = mod.m_mem_block_lst.begin();
                block_it != mod.m_mem_block_lst.end();
                block_it ++
            )
            {
                CMem_block & block = block_it->second;
                if(block.m_start_addr)
                {
                    free(block.m_start_addr);
                    block.m_start_addr = 0;
                    mod.m_current_memory_usage -= block.m_raw_size;
                }
            }
        this->m_modules.erase(mod_it);
        this->hips_memmgr_mod_unlock();
        return MEMBLOCK_LEAK_FOUND;
    }
    else
    {
        this->hips_memmgr_mod_unlock();
        this->m_modules.erase(mod_it);
        return 0;
    }
}

bool CHips_memmgr::hips_memmgr_mod_lock()
{
    return this->m_module_mutex.lock_mutex();
}

bool CHips_memmgr::hips_memmgr_mod_unlock()
{
    return this->m_module_mutex.unlock_mutex();
}
/*
  str_mod_name: the name of the detection mod who will request memory block;
  *phandle : output paramter, the pointer to receive the handle gen by hips_memmgr mode, if function faild the handle is 0
  max_mem_usage: the max memory usage for the mod, if all memory size of the mod using exceed the value ,the memory alloc request will failed.  
  return : error code, if not any error occured the return value is 0, 
*/
handle CHips_memmgr::registe(const string &str_mod_name, uint32 max_mem_usage, OUT uint32 *perror)
{
    handle mem_handle = 0;
    if (str_mod_name.length() == 0)
    {
        if ( perror)
            *perror =  INVALID_PARAMETER;
        return 0;
    }
    CHips_memmgr_mod new_mod;

    this->hips_memmgr_mod_lock();
    while (1)
    {
        // if not any mod registed
        if (this->m_modules.size() == 0)
        {
            mem_handle = 1;
            CHips_memmgr_mod modobj(mem_handle, str_mod_name, max_mem_usage);
            new_mod = modobj;
            break;
        }

        // if same mod name already registed, return the exist handle only.
        for (mod_map::iterator it = this->m_modules.begin(); it != this->m_modules.end(); it++)
        {
            if (it->second.get_name() == str_mod_name)
            {
                mem_handle = it->first;
                if(perror)
                    *perror = 0;
                this->hips_memmgr_mod_unlock();
                return mem_handle;
            }
        }

        mod_map::iterator it = this->m_modules.begin();
        while (1)
        { // The aim of the the loop is to resuse the handle value which has been unregisted.
            handle pre_handle = it->first;
            it++;
            if (it != this->m_modules.end())
            {
                if (it->first - pre_handle > 1)
                {
                    mem_handle = it->first - 1;
                    CHips_memmgr_mod modobj(mem_handle, str_mod_name, max_mem_usage);
                    new_mod = modobj;
                    break;
                }
            }
            else
            {
                break;
            }
        }
        if (it == this->m_modules.end())
        {
            mem_handle = it->first + 1;
            CHips_memmgr_mod modobj = CHips_memmgr_mod(mem_handle, str_mod_name, max_mem_usage);
            new_mod = modobj;
        }
        break;
    }

    pair<mod_map::iterator, bool> ret =
        this->m_modules.insert(pair <handle, CHips_memmgr_mod> (mem_handle, new_mod));
    
    this->hips_memmgr_mod_unlock();
    
    if (!ret.second)
    {
        //except event
    }

    if(perror)
        *perror = 0;

    return mem_handle;
}

bool CHips_memmgr_mod::mem_block_lock()
{
    return m_mem_block_mutex.lock_mutex();
}
bool CHips_memmgr_mod::mem_block_unlock()
{
    return m_mem_block_mutex.unlock_mutex();
}
void *CHips_memmgr::hips_memmgr_malloc(handle mem_handle, size_t size, OUT uint32 *perror )
{
    mod_map::iterator mod_it = this->m_modules.find(mem_handle);
    if (mod_it != this->m_modules.end())
    {
        CHips_memmgr_mod & mod_obj = mod_it->second;
        if (mod_obj.m_current_memory_usage + size > mod_obj.m_max_memory_usage)
        { //memeory usage will exceed the max limit
            if (perror)
                *perror = OVER_LIMIT;
            return 0;
        }
        else
        {
            if(!mod_obj.mem_block_lock())
            {
                if (perror)
                    *perror =MEMBLOCK_FATAL_ERROR;
                return 0; 
            }

            size_t raw_size = size;
            if (raw_size % MEMORY_ALIGNMENT != 0)
            {
                raw_size = raw_size + (MEMORY_ALIGNMENT - (raw_size % MEMORY_ALIGNMENT));
            }
            raw_size = raw_size + MEMORY_ALIGNMENT * 2;

            void * addr = malloc(raw_size);
            if (addr)
            {
                memset(addr, 0, raw_size);

                /*The magical number for detecting memory block upside overstep  */
                ((char *)addr)[0] = 'l';
                ((char *)addr)[1] = 'i';
                ((char *)addr)[2] = 'u';
                ((char *)addr)[3] = 'z';
                ((char *)addr)[4] = 'h';
                ((char *)addr)[5] = 'h';
                ((char *)addr)[6] = 'u';
                ((char *)addr)[7] = 'a';
                char *p = ((char *)addr + MEMORY_ALIGNMENT + size);
                /*The magical number for detecting memory block underside overstep */
                ((char *)p)[0] = 'a';
                ((char *)p)[1] = 'u';
                ((char *)p)[2] = 'h';
                ((char *)p)[3] = 'h';
                ((char *)p)[4] = 'z';
                ((char *)p)[5] = 'u';
                ((char *)p)[6] = 'i';
                ((char *)p)[7] = 'l';

                CMem_block mem_block(addr, (void *)((byte*)addr + MEMORY_ALIGNMENT), size, raw_size);
                pair<memblock_lst::iterator, bool> ret = 
                mod_obj.m_mem_block_lst.insert(pair<uint64, CMem_block> ((uint64) ((byte*)addr + MEMORY_ALIGNMENT), mem_block));
               if (!ret.second)
                {
                    //exp
                    /*
                    This situation means some mod using other method free the memory block which allcated by CHips_memmgr. 
                    */
                    if (perror)
                        *perror = MEMBLOCK_FATAL_ERROR;
                    mod_obj.mem_block_unlock();
                    return 0;
                }
                else
                {
                    if (perror)
                        *perror = 0;

                    mod_obj.m_current_memory_usage += raw_size;
                    mod_obj.mem_block_unlock();
                    return mem_block.m_allocated_start_addr;
                }

            }
            else
            {
                if (perror)
                    *perror = MEMBLOCK_FATAL_ERROR;
                mod_obj.mem_block_unlock();
                return 0;
            }
        }
    }
    else
    {
        if (perror)
            *perror = UNKNOWN_MOD;
        return 0;
    }
    return 0;
}

void CHips_memmgr::hips_memmgr_free(handle mem_handle, void * buffer, uint32 *perror)
{
    mod_map::iterator mod_it = this->m_modules.find(mem_handle);
    if (mod_it != this->m_modules.end())
    {
        CHips_memmgr_mod & mem_mod = mod_it->second;
        mem_mod.mem_block_lock();
        memblock_lst::iterator memblock_it = mem_mod.m_mem_block_lst.find((uint64)buffer);
       if (memblock_it != mem_mod.m_mem_block_lst.end())
        {
            CMem_block & mem_block = memblock_it->second;
            if(
               strncmp((char *)mem_block.m_start_addr, "liuzhhua", MEMORY_ALIGNMENT) != 0 
               ||
               strncmp((char*)mem_block.m_start_addr+ MEMORY_ALIGNMENT + mem_block.m_size ,  
                         "auhhzuil", MEMORY_ALIGNMENT) != 0
              )
              {
                  // found memory overstep 
                if(perror)
                    *perror = MEMBLOCK_OVERSTEP;
                  
              }
                free(mem_block.m_start_addr);
                mem_mod.m_current_memory_usage -= mem_block.m_raw_size;
                mem_mod.m_mem_block_lst.erase(memblock_it);
                mem_mod.mem_block_unlock();
                return;
        }
        else
        {
            if (perror)
                *perror = INVALIDE_MEMBLOCK;
            mem_mod.mem_block_unlock();
        }
    }
    else
    {
        if (perror)
            *perror = UNKNOWN_MOD;
    }
    
}
 CHips_memmgr::~CHips_memmgr()
{
}

int CHips_memmgr::hips_memmgr_query_usage(char *pstr_buf, IN OUT uint32* psize_in_bytes)
{
    string info="";
    stringstream outstream;
    size_t total_usage =0;
    for (mod_map::iterator mod_it = this->m_modules.begin(); mod_it != this->m_modules.end(); mod_it++)
    {
        CHips_memmgr_mod & mod = mod_it->second;
        total_usage += mod.m_current_memory_usage;
        outstream << "module name:" << mod.m_mod_name << ',';
        outstream << "usage:" << mod.m_current_memory_usage << endl;

        for (memblock_lst::iterator mem_it = mod.m_mem_block_lst.begin(); mem_it != mod.m_mem_block_lst.end(); mem_it ++ )
        {
            CMem_block & block = mem_it->second; 
            outstream << "start addr:" << showbase << hex << (uint64) block.m_start_addr << ',';
            outstream << "size:" << dec << block.m_size << ",";
            outstream << "raw size:" << block.m_raw_size << endl;
        }
        outstream << endl;
    }
    outstream << "total usage:" << hex << total_usage << endl;

    if ( pstr_buf == 0 )
    {
        *psize_in_bytes = 0;
        return outstream.str().length();
    }
    else
    {
        if(*psize_in_bytes - 1 >= outstream.str().length())
        {
            strncpy(pstr_buf, outstream.str().c_str(), outstream.str().length());
            *psize_in_bytes = outstream.str().length();
            return 0;
        }
        else
        {
            strncpy(pstr_buf, outstream.str().c_str(), *psize_in_bytes - 1);
            *psize_in_bytes = *psize_in_bytes - 1;
            return outstream.str().length();
        }
        

    }

}

    CHips_memmgr_mod::CHips_memmgr_mod()
    {
        m_handle = 0;
        m_mod_name = "";
        m_max_memory_usage = 0;
        m_mem_block_mutex.init_mutex();
    }

    CHips_memmgr_mod::CHips_memmgr_mod(handle mod_handle, string mod_name, size_t max_memory_usage):
    m_handle(mod_handle),
    m_mod_name(mod_name),
    m_max_memory_usage(max_memory_usage)
    {
        m_mem_block_mutex.init_mutex();
    }

    CHips_memmgr_mod::CHips_memmgr_mod(const CHips_memmgr_mod& obj)
    {
        m_handle = obj.m_handle;
        m_mod_name = obj.m_mod_name;
        m_max_memory_usage = obj.m_max_memory_usage;
        m_mem_block_mutex = obj.m_mem_block_mutex;
    }

    CHips_memmgr_mod::~CHips_memmgr_mod()
    {
/*        this->mem_block_lock();
        for (memblock_lst::iterator block_it = m_mem_block_lst.begin(); 
             block_it != m_mem_block_lst.end();
             block_it ++
            )
            {
                CMem_block & mem_block = block_it->second;
                free(mem_block.m_start_addr);
            }
        this->mem_block_unlock();
*/
    }
    CHips_memmgr_mod & CHips_memmgr_mod::operator=( const CHips_memmgr_mod &robj )
    {
       if (this == &robj)
            return *this;
        
        this->m_handle = robj.m_handle;
        this->m_mod_name = robj.m_mod_name;
        this->m_max_memory_usage = robj.m_max_memory_usage;
        this->m_mem_block_mutex = robj.m_mem_block_mutex;
        return *this;
    }

    string CHips_memmgr_mod::get_name()
    {
        return this->m_mod_name;
    };
