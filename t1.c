extern unsigned char __heap_base;

typedef unsigned int uint;


//External functions for allocator
uint base_heap_pointer = (uint)&__heap_base;

char* get_memory_base(){
  return (char*)base_heap_pointer;
}

uint get_page_size(){
  return 64 * 1024;
}

//A little less than a GB
uint get_max_possible_memory_size(){
  return 3U * 1024U * 1024U * 1024U - base_heap_pointer;
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

//Allocator allocation functions
extern void* alloc_mem(SIZE_TYPE mem);
extern void free_mem(void* mem_ptr);

#define nullptr ((void*)0)

//External js logging function
extern void lognum(int val);
extern void log_c_str(const char* cstr);

//Canvas interface codes
typedef union Color Color;
union Color {
  struct{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
  };

};


#define _countof(arr) (sizeof(arr)/sizeof((arr)[0]))
#define min(a,b) (((a) < (b))?(a):(b))
const Color rotations[] = {
  {255,255,255,255},
  {255,0,0,255},
  {0,255,0,255},
  {0,0,255,255},
  {0,0,0,255},
  {255,255,0,255},
  {255,0,255,255},
  {0,255,255,255}
};
int offset = 0;
int inx = 0;

void update_canvas(int width, int height, Color image[]){
#define img(i,j) (image[i*width + j])
  for(int i = 0; i < height; ++i)
    for(int j = 0; j < width; ++j)
      img(i,j) = rotations[inx];
  
      
  for(int j = offset; j < offset + width/3; ++j){
    for(int i = offset; i < offset +  height/3; ++i){
      img(i,j) = (Color){127,127,127,255};
    }
  }
  inx = (inx + 1) %_countof(rotations);
  offset+=1;
  if((offset + min(width,height)/3) > (min(width,height)))
    offset = 0;
#undef img  
}
