
#ifndef SIZE_TYPE
#define SIZE_TYPE unsigned long long
#endif

#define nullptr ((void*)0)
#define array_len(arr) (sizeof(arr)/ sizeof((arr)[0]))

//Make a bit map allocator , each block with 2 bits information
//Block size is fixed, and always rounded up, may cause loss of memory, but is easier to maintain

enum  BlockState{
  BLK_FREE = 0,
  BLK_USED = 1,
  BLK_EXTEND = 2,
};

/* char _memory[1024 * 1024 * 10] = {0}; */
/* const SIZE_TYPE page_size = 128; */


extern char* get_memory_base();
extern SIZE_TYPE get_page_size();
extern SIZE_TYPE get_max_possible_memory_size();
extern void grow_memory_by_page(int pages);

static SIZE_TYPE used_mem = 0;


//Allocate some pages, in that allocate some regions for bitmap and next pointer of bitmap
enum{
  BLOCK_SIZE = 16,
  MAP_BLKS = 256,
  BITS_ARR_LEN=((MAP_BLKS * BLOCK_SIZE - sizeof(char*) + sizeof(SIZE_TYPE) - 1) / sizeof(SIZE_TYPE)),
  DATA_PER_MAP = ((BITS_ARR_LEN * sizeof(SIZE_TYPE)) / 2),
  MAX_ALLOC_SIZE = BLOCK_SIZE * DATA_PER_MAP,
};

typedef struct BitMap BitMap;
struct BitMap {
  SIZE_TYPE bitmap[BITS_ARR_LEN];
  BitMap* next_map;
};

static BitMap* first_map = nullptr;

typedef struct LargeMem LargeMem;
struct LargeMem {
  char* ptr;
  SIZE_TYPE size;
  LargeMem* next;
};
static LargeMem* large_first_free = nullptr;
static LargeMem* large_first_allocd = nullptr;

static LargeMem* insert_large_mem_by_size(LargeMem* head, LargeMem* new_one){

  LargeMem** phead = &head;
  for(;(*phead) != nullptr; (*phead) = (*phead)->next){
    if((*phead)->size > new_one->size)
      break;
  }

  new_one->next = (*phead);
  (*phead) = new_one;
  return head;  
}

void* alloc_mem(SIZE_TYPE mem);
void free_mem(void* mem_ptr);
static void* alloc_large_mem(SIZE_TYPE size){
  size = ((size + sizeof(BitMap) + MAX_ALLOC_SIZE - 1) / (sizeof(BitMap) + MAX_ALLOC_SIZE)) *
    (sizeof(BitMap) + MAX_ALLOC_SIZE);

  //Find one in the free list if there
  LargeMem** phead = &large_first_free;
  for(; (*phead) != nullptr; phead = &((*phead)->next)){
    if((*phead)->size >= size){
      break;
    }
  }

  if((*phead) == nullptr){
    
    LargeMem* new_large = alloc_mem(sizeof*new_large);
    if(new_large == nullptr){
      return nullptr;
    }
    //First also align up used_size to max alloc size and bitmap size
    SIZE_TYPE total_mem_per_map = sizeof(BitMap) + MAX_ALLOC_SIZE;
    SIZE_TYPE aligned_used_mem = ((used_mem + total_mem_per_map - 1)/total_mem_per_map) *
      total_mem_per_map;

    SIZE_TYPE new_used_mem = aligned_used_mem + size;
    new_used_mem = (new_used_mem + get_page_size() - 1) & ~(get_page_size() - 1);
    if(new_used_mem > get_max_possible_memory_size()){
      free_mem(new_large);
      return nullptr;
    }

    //Bump memory
    grow_memory_by_page((new_used_mem - used_mem) / get_page_size());
    used_mem = new_used_mem;

    
    new_large->next = nullptr;
    new_large->size = size;
    new_large->ptr = (get_memory_base() + aligned_used_mem);
    (*phead) = new_large;
  }

  //Remove from free list and add to not free list
  LargeMem* mem = (*phead);
  (*phead) = mem->next;
  mem->next = nullptr;

  mem->next = large_first_allocd;
  large_first_allocd = mem;
  
  return mem->ptr;
}
#define false 0
#define true 1
static _Bool free_large_mem(void* ptr){
  //Find one in the free list if there
  LargeMem** phead = &large_first_allocd;
  for(; (*phead) != nullptr; phead = &((*phead)->next)){
    if((*phead)->ptr == (char*)ptr){
      break;
    }
  }

  if((*phead) == nullptr)
    return false;

  LargeMem* mem_node = (*phead);
  (*phead) = mem_node->next;

  large_first_free = insert_large_mem_by_size(large_first_free, mem_node);
  return true;
}


static BitMap* one_more_map(){
  //TODO::Check, if possible, allocate from 'large memory' 
  
  //First also align up used_size to max alloc size and bitmap size
  SIZE_TYPE total_mem_per_map = sizeof(BitMap) + MAX_ALLOC_SIZE;
  SIZE_TYPE aligned_used_mem = ((used_mem + total_mem_per_map - 1)/total_mem_per_map) *
    total_mem_per_map;

  SIZE_TYPE new_used_mem = aligned_used_mem + sizeof(BitMap);
  new_used_mem = (new_used_mem + get_page_size() - 1) & ~(get_page_size() - 1);
  
  if(new_used_mem > get_max_possible_memory_size())
    return nullptr;

  //Bump memory
  grow_memory_by_page((new_used_mem - used_mem) / get_page_size());
  used_mem = new_used_mem;

    
  BitMap* next_thing =(BitMap*)(get_memory_base() + aligned_used_mem);
  (*next_thing) = (BitMap){0};
  //Link next_thing to the end of current linked list
  BitMap** headptr = &first_map;
  while((*headptr) != nullptr){
    headptr = &((*headptr)->next_map);
  }
  (*headptr) = next_thing;
  return next_thing;
}

static enum BlockState get_nth(SIZE_TYPE* arr, SIZE_TYPE inx){
  SIZE_TYPE blk_size = sizeof(SIZE_TYPE)/2;
  SIZE_TYPE blk = inx / blk_size;
  inx = inx % blk_size;
  return (enum BlockState){ (arr[blk] >> (inx * 2) ) & 0b11 };
}

static void set_nth(SIZE_TYPE* arr, SIZE_TYPE inx, enum BlockState value){
  SIZE_TYPE blk_size = sizeof(SIZE_TYPE)/2;
  SIZE_TYPE blk = inx / blk_size;
  inx = inx % blk_size;
  SIZE_TYPE bits = (0b11 << (2 * inx));
  SIZE_TYPE val = ((SIZE_TYPE)value << (2 * inx));
  //First reset , then set
  arr[blk] = arr[blk] & ~bits;
  arr[blk] = arr[blk] | val;
}

//Large allocations: how they will work?

void* alloc_mem(SIZE_TYPE mem){
  mem = (mem + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
  //assert((mem <= MAX_ALLOC_SIZE) && "Large allocations not implemented yet");
  if(mem > MAX_ALLOC_SIZE){
    return alloc_large_mem(mem);
  }
  
  SIZE_TYPE blk_count = mem / BLOCK_SIZE;
  
  BitMap* map = first_map;
  SIZE_TYPE inx = 0;
  for(; map != nullptr; map = map->next_map){

    for(inx = 0; inx < DATA_PER_MAP;){

      //Find the length of free at this place
      SIZE_TYPE free_len = 0;
      for(SIZE_TYPE i = inx; i < DATA_PER_MAP; ++i){
	if(get_nth(map->bitmap, i) != BLK_FREE)
	  break;
	free_len++;
      }
      if((free_len * BLOCK_SIZE) >= mem)
	goto exit_loop;
      if(free_len == 0)
	free_len++;
      inx += free_len;
    }

  }
 exit_loop:
  if(map == nullptr){
    map = one_more_map();
    inx = 0;
    if(map == nullptr)
      return nullptr;
  }

  //Find if enough pages is already requested
  //Convert inx + map to pointer
  char* ptr = (char*)map + sizeof(*map) + inx * BLOCK_SIZE;
  if(((mem + ptr) > (get_memory_base() + used_mem))){
    //Request enough mem pages
    SIZE_TYPE new_mem = ((mem + ptr) - (get_memory_base() + used_mem));
    new_mem = (new_mem + get_page_size() - 1) & ~(get_page_size() - 1);
    SIZE_TYPE new_pages = new_mem / get_page_size();
    grow_memory_by_page(new_pages);
    used_mem += new_pages * get_page_size();

  }

  //Set bits in corresponding
  for(SIZE_TYPE i = inx; i < (inx + (mem / BLOCK_SIZE)); ++i){
    set_nth(map->bitmap, i, BLK_EXTEND);
  }
  set_nth(map->bitmap, inx, BLK_USED);
  return ptr;
}

void free_mem(void* mem_ptr){

  if(free_large_mem(mem_ptr)){
    return;
  }
  
  char* to_free = mem_ptr;
  //Find mem_ptr in one of linked list
  BitMap* map = first_map;
  for(; map != nullptr; map = map->next_map){
    if((to_free >= ((char*)map + sizeof(*map))) &&
       (to_free < ((char*)map + sizeof(*map) + MAX_ALLOC_SIZE))){
      break;
    }
  }

  //Invalid memory
  if(map == nullptr){
    return;
  }

  //Now test if it is a valid memory head
  SIZE_TYPE inx = to_free - ((char*)map + sizeof(*map));
  inx = inx / BLOCK_SIZE;
  if(get_nth(map->bitmap, inx) != BLK_USED)
    return;

  //Also test if this memory is absolute start of the block
  if(to_free != ((char*)(map + 1) + inx * BLOCK_SIZE))
    return;

  set_nth(map->bitmap, inx, BLK_FREE);
  inx++;
  for(; inx < DATA_PER_MAP; ++inx){
    if(get_nth(map->bitmap, inx) != BLK_EXTEND)
      break;
    set_nth(map->bitmap, inx, BLK_FREE);
  }
  
}
