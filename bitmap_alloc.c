
#ifndef SIZE_TYPE
//#define SIZE_TYPE unsigned long long
#endif

#define nullptr ((void*)0)
#define array_len(arr) (sizeof(arr)/ sizeof((arr)[0]))

#define false 0
#define true 1
typedef _Bool bool;
//Make a bit map allocator , each block with 2 bits information
//Block size is fixed, and always rounded up, may cause loss of memory, but is easier to maintain



enum  BlockState{
  BLK_FREE = 0,
  BLK_USED = 1,
  BLK_EXTEND = 2,
};

/* char _memory[1024 * 1024 * 10] = {0}; */
/* const SIZE_TYPE page_size = 128; */

//Allocate some pages, in that allocate some regions for bitmap and next pointer of bitmap
enum{
  BLOCK_SIZE = 16,
  MAP_BLKS = 64,
  BITS_ARR_LEN=((MAP_BLKS * BLOCK_SIZE - sizeof(char*)) / sizeof(SIZE_TYPE)),
  DATA_PER_MAP = ((BITS_ARR_LEN * sizeof(SIZE_TYPE) * 4)),
  MAX_ALLOC_SIZE = BLOCK_SIZE * DATA_PER_MAP,
};


typedef struct BitMap BitMap;
struct BitMap {
  SIZE_TYPE bitmap[BITS_ARR_LEN];
  BitMap* next_map;
};




static char* _memory_base = nullptr;
static SIZE_TYPE _page_size = 0;



extern SIZE_TYPE get_max_possible_memory_size();
extern void grow_memory_by_page(int pages);
extern void* memcpy(void* restrict dest, const void* restrict src, SIZE_TYPE count);

//This is the amount of memory taken already, is strictly <=
//_reserved_mem and is aligned to a map struct size + max map refrenceably memory size
static SIZE_TYPE _used_mem = 0;
//static SIZE_TYPE used_mem = 0;
//This is the amount of memory taken already and that is
//aligned to page boundary
static SIZE_TYPE _reserved_mem = 0;


#ifdef DEBUG
extern void log_c_str(const char* cstr);
extern void logint(int val);
SIZE_TYPE mem_allocr_query_struct_size(){
  return sizeof(BitMap);
}
SIZE_TYPE mem_allocr_query_max_alloc(){
  return MAX_ALLOC_SIZE;
}
SIZE_TYPE mem_allocr_query_mapping_size(){
  return BLOCK_SIZE * MAP_BLKS;
}
SIZE_TYPE mem_allocr_query_used_up(){
  return _used_mem;
}
SIZE_TYPE mem_allocr_query_reseved(){
  return _reserved_mem;
}
//Define exportable functions for debugging

#else
static void log_c_str(const char* cstr){
  (void)cstr;
}
static void logint(int val){
  (void)val;
}
#endif

//Can and should call this once on initialization
void allocator_initialize(char* memory_base, SIZE_TYPE page_size){
  if(_page_size == 0){
    _memory_base = memory_base;
    _page_size = page_size;
  }
}
static char* get_memory_base(){
  return _memory_base;
}
static SIZE_TYPE get_page_size(){
  return _page_size;
}


//Grows the memory , makes sure that reserved is >= used
static bool ensure_reserved(SIZE_TYPE used_upto){
  
  if(get_page_size() == 0){
    log_c_str("Zero memory page size provided");
    return false;
  }
  
  SIZE_TYPE new_res = (used_upto + get_page_size() - 1) & ~(get_page_size() - 1);

  if(new_res > _reserved_mem){
    if(new_res > get_max_possible_memory_size()){
      return false;
    }
    
    SIZE_TYPE more_pages = (new_res - _reserved_mem) /get_page_size();
    grow_memory_by_page(more_pages);
    _reserved_mem = new_res;
  }
  _used_mem = used_upto;
  return true;
}






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
  for(;(*phead) != nullptr; phead = &(*phead)->next){
    if((*phead)->size > new_one->size)
      break;
  }

  new_one->next = (*phead);
  (*phead) = new_one;
  return head;  
}

void* alloc_mem(SIZE_TYPE mem);
void free_mem(void* mem_ptr);

static LargeMem** find_intersecting(LargeMem* mem1, LargeMem** pstart){
  for(LargeMem** pmem2 = pstart;
      (*pmem2) != nullptr;){

    if(mem1->ptr > (*pmem2)->ptr){
      if((mem1->ptr - (*pmem2)->ptr) <= mem1->size){
	return pmem2;
      }
    }
    else{
      if(((*pmem2)->ptr - mem1->ptr) <= (*pmem2)->size){
	return pmem2;
      }
    }
    pmem2 = &((*pmem2)->next);
  }
  return nullptr;
}

static void try_merging_large_mems(void){

  //It will merge, but unfortunately, make the
  //linked list not in sized order
  //Need to sort the linked list if merged
  
  for(LargeMem* mem1 = large_first_free;
      (mem1 != nullptr) && (mem1->next != nullptr);
      mem1 = mem1->next){
    SIZE_TYPE prev_size = mem1->size;
    LargeMem** pmem2 = &mem1->next;
    while((pmem2 = find_intersecting(mem1, pmem2)) != nullptr){
      char* new_mem = nullptr;
      SIZE_TYPE new_size = 0;
      //We always remove *pmem2      
      //See if mem1 and *pmem2 are consecutive
      if(mem1->ptr > (*pmem2)->ptr){
	if((mem1->ptr - (*pmem2)->ptr) <= mem1->size){
	  new_mem = (*pmem2)->ptr;
	  new_size = (mem1->ptr - (*pmem2)->ptr) + mem1->size;
	}
      }
      else{
	if(((*pmem2)->ptr - mem1->ptr) <= (*pmem2)->size){
	  new_mem = mem1->ptr;
	  new_size = ((*pmem2)->ptr - mem1->ptr) + (*pmem2)->size;
	}
      }
      
      if(new_mem != nullptr){
	LargeMem* to_free = *pmem2;
	(*pmem2)->next = to_free->next;
	mem1->ptr = new_mem;
	mem1->size = new_size;
	free_mem(to_free);
      }
      else{
	pmem2 = &((*pmem2)->next);
      }
    }

    if(prev_size != mem1->size){
      //Merging happened, so need to resort, kind of memove 

      LargeMem* ptr = mem1;
      for(; (ptr->next != nullptr) &&
	    (ptr->next->size < ptr->size);
	  ptr = ptr->next){
	//Swap values of ptr and ptr->next except for pointer values
	LargeMem* next = ptr->next;
	LargeMem tmp = *ptr;

	*ptr = *next;
	*next = tmp;

	next->next = ptr->next;
	ptr->next = next;
      }
    }
  }


  
}

static void* alloc_large_mem(SIZE_TYPE size){
  log_c_str("Allocate large mem entered");
  size = ((size + sizeof(BitMap) + MAX_ALLOC_SIZE - 1) / (sizeof(BitMap) + MAX_ALLOC_SIZE)) *
    (sizeof(BitMap) + MAX_ALLOC_SIZE);

  //Find one in the free list if there
  LargeMem** phead = &large_first_free;
  for(; (*phead) != nullptr; phead = &((*phead)->next)){
    if((*phead)->size >= size){
      break;
    }
  }
  log_c_str("Now finished looking for existing");
  if((*phead) == nullptr){
    
    LargeMem* new_large = alloc_mem(sizeof*new_large);
    log_c_str("Allocd ptr for large mem");
    if(new_large == nullptr){
      return nullptr;
    }
    
    //First also align up used_size to max alloc size and bitmap size
    SIZE_TYPE total_mem_per_map = sizeof(BitMap) + MAX_ALLOC_SIZE;
    SIZE_TYPE aligned_used_mem = ((_used_mem + total_mem_per_map - 1)/total_mem_per_map) *
      total_mem_per_map;
    
    SIZE_TYPE new_used_mem = aligned_used_mem + size;
    log_c_str("New used mem is of value : ");
    logint((int)new_used_mem);
    //new_used_mem = (new_used_mem + get_page_size() - 1) & ~(get_page_size() - 1);

    //Bump memory
    //grow_memory_by_page((new_used_mem - used_mem) / get_page_size());
    //_used_mem = new_used_mem;
    //Okay to not check ensure i guess
    if(!ensure_reserved(new_used_mem)){
      free_mem(new_large);
      return nullptr;
    }
    log_c_str("Ensure add reserved");
    
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
  //Maybe make a realloc feature
  
  return true;
}


static BitMap* one_more_map(){
  //TODO::Check, if possible, allocate from 'large memory' 
  
  //First also align up used_size to max alloc size and bitmap size
  SIZE_TYPE total_mem_per_map = sizeof(BitMap) + MAX_ALLOC_SIZE;
  SIZE_TYPE aligned_used_mem = ((_used_mem + total_mem_per_map - 1)/total_mem_per_map) *
    total_mem_per_map;

  SIZE_TYPE new_used_mem = aligned_used_mem + sizeof(BitMap);
  //new_used_mem = (new_used_mem + get_page_size() - 1) & ~(get_page_size() - 1);

  //Bump memory
  //grow_memory_by_page((new_used_mem - used_mem) / get_page_size());
  //_used_mem = new_used_mem;
  if(!ensure_reserved(new_used_mem)){
    return nullptr;
  }

    
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
  
  //SIZE_TYPE blk_count = mem / BLOCK_SIZE;
  
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
    log_c_str("One more map passed as");
    logint((int)map);
     
    inx = 0;
    if(map == nullptr)
      return nullptr;
  }

  //Find if enough pages is already requested
  //Convert inx + map to pointer
  char* ptr = (char*)map + sizeof(*map) + inx * BLOCK_SIZE;
  if(((mem + ptr) > (get_memory_base() + _used_mem))){
    //Request enough mem pages
    SIZE_TYPE new_mem = ((mem + ptr) - (get_memory_base() + _used_mem));
    //new_mem = (new_mem + get_page_size() - 1) & ~(get_page_size() - 1);
    //SIZE_TYPE new_pages = new_mem / get_page_size();
    //grow_memory_by_page(new_pages);
    //used_mem += new_pages * get_page_size();
    //_used_mem += new_mem;
    if(!ensure_reserved(_used_mem + new_mem)){
      return nullptr;
    }
  }

  //Set bits in corresponding
  for(SIZE_TYPE i = inx; i < (inx + (mem / BLOCK_SIZE)); ++i){
    set_nth(map->bitmap, i, BLK_EXTEND);
  }
  set_nth(map->bitmap, inx, BLK_USED);
  return ptr;
}

//Returns if it is a map allocated block

static BitMap* find_if_map_allocd(void* mem_ptr, SIZE_TYPE* out_inx){
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
    return nullptr;
  }
  //Now test if it is a valid memory head
  SIZE_TYPE inx = to_free - ((char*)map + sizeof(*map));
  inx = inx / BLOCK_SIZE;
  if(get_nth(map->bitmap, inx) != BLK_USED)
    return nullptr;
  //Also test if this memory is absolute start of the block
  if(to_free != ((char*)(map + 1) + inx * BLOCK_SIZE))
    return nullptr;
  *out_inx = inx;
  return map;
}

void free_mem(void* mem_ptr){

  if(free_large_mem(mem_ptr)){
    return;
  }


  SIZE_TYPE inx = 0;
  BitMap* map = find_if_map_allocd(mem_ptr, &inx);
  if(map == nullptr)
    return;
  
  set_nth(map->bitmap, inx, BLK_FREE);
  inx++;
  for(; inx < DATA_PER_MAP; ++inx){
    if(get_nth(map->bitmap, inx) != BLK_EXTEND)
      break;
    set_nth(map->bitmap, inx, BLK_FREE);
  }
  
}


//For this the _reserved_mem and _used_mem might be necessary
static void* realloc_large_mem(LargeMem* memptr, SIZE_TYPE new_size){

  //TODO:: A bug is here, that is non conformant behaviour of realloc
  //Even after all this merging, if size isn't enough and need to increase ,
  //which fails, there should be no changes in the original format, which has happened
  //here 
  LargeMem** pmem2 = &large_first_free;
  while((pmem2 = find_intersecting(memptr, pmem2)) != nullptr){
    log_c_str("Going to do some realloc merging");
    char* new_mem = nullptr;
    SIZE_TYPE new_size = 0;
    //We always remove *pmem2      
    //See if memptr and *pmem2 are consecutive
    if(memptr->ptr > (*pmem2)->ptr){
      if((memptr->ptr - (*pmem2)->ptr) <= memptr->size){
	//Need to do some memmoving
	char* ptr1 = (*pmem2)->ptr;
	char* ptr2 = memptr->ptr;
	char* ptrn = memptr->ptr + memptr->size;

	while(ptr2 < ptrn){
	  if((ptrn - ptr2) > (ptr2 - ptr1)){
	    memcpy(ptr1, ptr2, ptr2-ptr1);
	  }
	  else{
	    memcpy(ptr1, ptr2, ptrn-ptr2);
	  }
	  SIZE_TYPE ptrdiff = ptr2 - ptr1;
	  ptr1 = ptr2;
	  ptr2 += ptrdiff;
	}
	new_mem = (*pmem2)->ptr;
	new_size = (memptr->ptr - (*pmem2)->ptr) + memptr->size;
      }
    }
    else{
      if(((*pmem2)->ptr - memptr->ptr) <= (*pmem2)->size){
	//no need for memmoving
	new_mem = memptr->ptr;
	new_size = ((*pmem2)->ptr - memptr->ptr) + (*pmem2)->size;
      }
    }
    LargeMem* to_free = *pmem2;
    (*pmem2) = to_free->next;
    memptr->ptr = new_mem;
    memptr->size = new_size;
    free_mem(to_free);
  }
  
  new_size = ((new_size + sizeof(BitMap) + MAX_ALLOC_SIZE - 1) /
	      (sizeof(BitMap) + MAX_ALLOC_SIZE)) *
    (sizeof(BitMap) + MAX_ALLOC_SIZE);

  //Find if aligned new size is same as old aligned size
  //for now also if lesser 
  if(new_size <= memptr->size){
    log_c_str("Large page realloc, same or less size");
    return memptr->ptr;
  }
  
  //Find if this is the recently allcocated and the last portion in memory, if
  //so, we may be able to just request more pages 
  //Find end of current page
  char* end_point = memptr->ptr + memptr->size;
  //When at end, end_point == mem_base + _used_up
  if(end_point == (get_memory_base() + _used_mem)){
    if(!ensure_reserved(_used_mem + new_size - memptr->size)){
      return nullptr;
    }
    memptr->size = new_size;
    return memptr->ptr;
  }
  
  
  //For now if more requested, just allocate it
  log_c_str("Before attempting large reallocation " );

  log_c_str("The allocated large pages " );
  for(LargeMem* ptr = large_first_allocd; ptr != nullptr; ptr = ptr->next){
    logint((int)(ptr->ptr));
  }
  log_c_str("The free large pages ");
  for(LargeMem* ptr = large_first_free; ptr != nullptr; ptr = ptr->next){
    logint((int)(ptr->ptr));
  }

  
  LargeMem* old_mem = memptr;
  void* new_mem = alloc_mem(new_size);
  if(new_mem == nullptr){
    log_c_str("Large page realloc, alloc larger page failed");
    return nullptr;
  }
  log_c_str("Larger page realloc successful as ");
  logint((int)new_mem);
  
  memcpy(new_mem, old_mem->ptr, old_mem->size);

  free_mem(old_mem->ptr);

  return new_mem;
}



void* realloc_mem(void* memptr, SIZE_TYPE new_size){
  log_c_str("The request for reallocation of :");
  logint((int)memptr);
  if(memptr == nullptr){
    log_c_str("Allocating during realloc");
    return alloc_mem(new_size);
  }

  if(new_size == 0){
    log_c_str("Freeing during realloc");
    free_mem(memptr);
    return nullptr;
  }
  log_c_str("Trying realloc , searching if large memory pages");
  
  //Find if it is a large memory

  
  LargeMem* head = large_first_allocd;
  for(; (head) != nullptr; head = ((head)->next)){
    logint((int)((head)->ptr));
    if((head)->ptr == (char*)memptr){
      break;
    }
  }

  
  
  
  
  if((head) != nullptr)
    return realloc_large_mem(head, new_size);
  log_c_str("Not a large alloc, realloc on bitmap");

  //Find if this is a bitmap allocated
  SIZE_TYPE inx;
  BitMap* map = find_if_map_allocd(memptr, &inx);

  if(map == nullptr)
    return nullptr;

  
  //Find it's size
  SIZE_TYPE size = 0;
  size++;
  for(SIZE_TYPE i = inx+1; i < DATA_PER_MAP; ++i){
    if(get_nth(map->bitmap, i) != BLK_EXTEND)
      break;
    size++;
  }

  size *= BLOCK_SIZE;


  new_size = (new_size + BLOCK_SIZE - 1) & ~(BLOCK_SIZE -1);
  //Memory increased beyond max_alloc_size
  if(new_size > MAX_ALLOC_SIZE){

    void* new_alloc = alloc_mem(new_size);
    if(new_alloc == nullptr)
      return nullptr;

    memcpy(new_alloc, memptr, size);
    free_mem(memptr);
    return new_alloc;
  }

  //Memory same
  if(new_size == size)
    return memptr;

  //Memory increased
  if(new_size > size){
    //SIZE_TYPE diff = new_size - size;
    //diff /= BLOCK_SIZE;
    for(SIZE_TYPE i = inx + size / BLOCK_SIZE; i < new_size / BLOCK_SIZE; ++i){
      if(get_nth(map->bitmap, i) != BLK_FREE){
	//Need to allocate more memory
	void* new_mem = alloc_mem(new_size);
	if(new_mem == nullptr)
	  return nullptr;

	memcpy(new_mem, memptr, size);
	free_mem(memptr);
	return new_mem;
      }
    }
    for(SIZE_TYPE i = inx + size / BLOCK_SIZE; i < new_size / BLOCK_SIZE; ++i){
      set_nth(map->bitmap, i,  BLK_EXTEND);
    }
    return memptr;

  }

  //Memory decreased
  SIZE_TYPE diff = size - new_size;
  diff = diff / BLOCK_SIZE;

  for(SIZE_TYPE i = inx + (size/BLOCK_SIZE - diff); i < size/BLOCK_SIZE; ++i){
    set_nth(map->bitmap, i, BLK_FREE);
  }
  return memptr;
}
