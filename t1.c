#include "t1.h"

extern void allocator_initialize(char* memory_base, SIZE_TYPE page_size);
//Initialize function 
void init_wasm(){
  allocator_initialize((char*)(&__heap_base), 64*1024);
}

//External functions for allocator
//A little less than a GB
uint get_max_possible_memory_size(){
  return 3U * 1024U * 1024U * 1024U - (uint)(&__heap_base);
}


extern void grow_memory_by_page(int pages);

//Also needed for allocator 
void* memset(void* base, int value, uint num){
  char* ptr = base;
  for(uint i = 0; i < num; ++i)
    ptr[i] = value;
  return ptr;
}
void* memcpy(void* restrict dest, const void* restrict src, uint count){
  char* cdest = dest;
  const char* csrc = src;
  for(uint i = 0; i < count; ++i)
    cdest[i] = csrc[i];
  
  return dest;
}

size_t strlen(const char* str){
  size_t count = 0;
  while(*str){
    str++;
    count++;
  }
  return count;
}
