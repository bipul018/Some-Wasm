extern unsigned char __heap_base;
typedef unsigned int uint;

//Allocator provided functions 
extern void* alloc_mem(SIZE_TYPE mem);
extern void free_mem(void* mem_ptr);
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
