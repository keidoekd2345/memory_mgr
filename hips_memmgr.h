#ifndef _hips_memmgr_h
#define _hips_memmgr_h
#include <string>
#include <map>
#include <list>
#include <vector>
using namespace std;
#include "threadobj.h"

#define uint32 unsigned int 
#define int32 int
#define int64 long long
#define uint64 unsigned long long
#define byte unsigned char 
#define dword uint32
#define qword uint64
#define handle uint32
#define IN
#define OUT
#define MEMORY_ALIGNMENT 8

/*********error code *************/
#define INVALID_PARAMETER  0x80000001
#define UNKNOWN_MOD 0X80000002
#define OVER_LIMIT  0X80000003
#define INVALIDE_MEMBLOCK 0X80000004
#define MEMBLOCK_OVERSTEP 0X80000005
#define MEMBLOCK_FATAL_ERROR 0X80000006
#define MEMBLOCK_LEAK_FOUND 0x80000007
class CMem_block
{
    public:
    CMem_block():m_start_addr(0),m_allocated_start_addr(0),m_size(0),m_raw_size(0){};
    CMem_block(void * start, void* allocated_start, size_t size, size_t raw_size):
    m_start_addr(start),
    m_allocated_start_addr(allocated_start),
    m_size(size),
    m_raw_size(raw_size){};

    CMem_block(const CMem_block & block)
    {
        m_start_addr = block.m_start_addr;
        m_allocated_start_addr= block.m_allocated_start_addr;
        m_size = block.m_size;
        m_raw_size = block.m_raw_size;
    }
    CMem_block & operator=( const CMem_block & robj)
    {
       if (this == &robj)
        return *this;

        this->m_start_addr = robj.m_start_addr;
        this->m_allocated_start_addr= robj.m_allocated_start_addr;
        this->m_size = robj.m_size;
        this->m_raw_size = robj.m_raw_size;
        return *this;
    }
    void * m_start_addr;
    void * m_allocated_start_addr;
    size_t m_size;
    size_t m_raw_size;  
};
class CHips_memmgr_mod
{
    #define memblock_lst map<uint64, CMem_block>
    public:
    CHips_memmgr_mod();
    CHips_memmgr_mod(handle mod_handle, string mod_name, size_t max_memory_usage);

    CHips_memmgr_mod(const CHips_memmgr_mod& obj);

    ~CHips_memmgr_mod();
    CHips_memmgr_mod &operator=( const CHips_memmgr_mod &robj );
    string get_name();
    bool mem_block_lock();
    bool mem_block_unlock();
    handle m_handle;
    string m_mod_name;
    size_t m_max_memory_usage;
    size_t m_current_memory_usage;
    CThread_mutex m_mem_block_mutex;
    memblock_lst m_mem_block_lst;
};

class CHips_memmgr
{
    #define mod_map map <handle, CHips_memmgr_mod>
    public:
    CHips_memmgr();
    /*
    str_mod_name: the name of the detection mod who will request memory block;
    *perror :output paramter, the pointer to receive error code, if not any error occured the  value is 0, 
    return :  the handle gen by hips_memmgr mode, if function faild the handle is 0 
    */
    handle registe(const string &str_mod_name, uint32 max_mem_usage, OUT uint32 *perror = 0);
    /*
      mod_handle: user call registe method and return the handle
      return : if mod_handle is a invalid handle the return value is INVALID_PARAMETER
               if the mod don't free all memory block, the return is MEMBLOCK_LEAK_FOUND
               other return 0;
    */
    
    int unregiste(handle mod_handle);
    void * hips_memmgr_malloc(handle mem_handle, size_t size, uint32 *perror = 0);

    void hips_memmgr_free(handle mem_handle, void * buffer, uint32 *perror = 0);


    /*
     pstr_buf: the character buffer to recieve the usage info, if the buffer is NULL,
                the function return really length of the buffer needed.
     psize_in_bytes: IN, the buffer length in byte, OUT, return the length of info copy to the buffer.
     return: if the input buffer length is not enough , the return value is the really length in bytes of the buffer needed
             Other ,return 0;
             user can 

    */
    int hips_memmgr_query_usage(char *pstr_buf, IN OUT uint32* psize_in_bytes);

    ~CHips_memmgr();
    private:
    mod_map m_modules;
    CThread_mutex m_module_mutex;
    bool hips_memmgr_mod_lock();
    bool hips_memmgr_mod_unlock();
};

#endif