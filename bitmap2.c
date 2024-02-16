#include <stdlib.h>
#include <stdio.h>

char _memory[16 * 1024 * 1024];
const size_t _page_size = 512;
size_t pages_used = 0;
char* get_memory_base(){
  return _memory;
}
unsigned long long get_page_size(){
  return _page_size;
}
unsigned long long get_max_possible_memory_size(){
  return sizeof(_memory);
}
void grow_memory_by_page(int pages){
  pages_used += pages;
  printf("Grow memory called\n");
}
#define SIZE_TYPE unsigned long long

void* alloc_mem(SIZE_TYPE mem);
void free_mem(void* mem_ptr);

int main(void){
  char* ptr1 = alloc_mem(161);
  ptr1[0] = 92;
  double* ptr3 = alloc_mem(_page_size);
  double* ptr2 = alloc_mem(10 * sizeof * ptr2);
  ptr2[0] = 2.4;
  ptr2[9] = 31.39;

  printf("Done\n");
  printf("Address of base : %p\n", _memory);
  printf("Two allocated addresses : %p and %p\n", ptr1, ptr2);
  printf("Offsets from base : %llu and %llu\n", ptr1-_memory, (char*)ptr2 - _memory);
  printf("Pages used up : %d\n", (int)pages_used);
  return 0;

}
	 
  
