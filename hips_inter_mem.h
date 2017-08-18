#ifndef __HIPS_INTER_MEM_H__
#define __HIPS_INTER_MEM_H__
//__attribute__((constructor)) ()
#define MAX_MOD_MEM_USAGE 1024*1024*500
unsigned int hips_i_mem_registe(const char * str_mod_name, unsigned int max_mem_usage);
unsigned int hips_i_mem_unregiste(unsigned int hhandle);
void * hips_i_mem_malloc(unsigned int mem_handle, size_t size);
void hips_i_mem_free(void * buffer);
#endif