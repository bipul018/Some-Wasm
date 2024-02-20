extern unsigned char __heap_base;
typedef unsigned int uint;

//Allocator provided functions 
void* alloc_mem(SIZE_TYPE mem);
void free_mem(void* mem_ptr);
void* realloc_mem(void* prev_ptr, SIZE_TYPE new_size);

void* memset(void* base, int value, uint num);
void* memcpy(void* restrict dest, const void* restrict src, uint count);
#define nullptr ((void*)0)


//External js logging function
void logint(int val);
void log_c_str(const char* cstr);
typedef SIZE_TYPE size_t;
typedef _Bool bool;
#define true ((bool)1)
#define false ((bool)0)

static void dbg_log_c_str(const char* cstr){
#ifdef DEBUG
  log_c_str(cstr);
#else
  (void)cstr;
#endif
}

static void dbg_logint(int val){
#ifdef DEBUG
  logint(val);
#else
  (void)val;
#endif
}
