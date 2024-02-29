#include "t1.h"

typedef struct BayesNode BayesNode;
struct BayesNode {
  const char* node_name; //Needs this to be heap allocated, will free at end
  size_t parent_count;
  //A probability distribution showing 2 ^ parent_count of possibilites in which this is true
  float* prob_dist;
  int* parents;
  int node_id;  //Id same with the UI element id
};

size_t get_bayes_node_size(){
  return sizeof(BayesNode);
}


bool need_sorting = true;
BayesNode* all_nodes = nullptr;
size_t node_count = 0;

//-1 implies failure to create new node
int create_new_node(const char* node_name){
  //dbg_log_c_str("Need to create node : ");
  //dbg_log_c_str(node_name);
  //dbg_logint((int)node_name);
  //dbg_log_c_str("Logging all the all_nodes first");
  //dbg_logint((int)all_nodes);
  //dbg_logint((int)(all_nodes + node_count));
  //dbg_log_c_str("all_nodes");
  for(size_t i = 0; i < node_count; ++i){
    //dbg_log_c_str(all_nodes[i].node_name);
    //dbg_logint(all_nodes[i].node_id);
  }
  //dbg_log_c_str("Now finished logging previous");
    
  BayesNode* added_node = realloc_mem(all_nodes,
				      (node_count + 1) *
				      sizeof(BayesNode));
  if(added_node == nullptr)
    return -1;
  added_node->prob_dist = alloc_mem(sizeof(float));
  if(added_node != all_nodes){
    //dbg_log_c_str("Pointer changed");
  }
  all_nodes = added_node;

  added_node = all_nodes + node_count;
  node_count++;

  added_node->node_id = node_count - 1;
  added_node->node_name = node_name;
  added_node->parent_count = 0;
  added_node->parents = nullptr;
  added_node->prob_dist = alloc_mem(sizeof(float));
  if(added_node->prob_dist == nullptr){
    log_c_str("Allocation error in allocating single probability item, Ignoring for now");
  }
  else{
    added_node->prob_dist[0] = 1.f;
  }
    
  //dbg_log_c_str("Logging all the all_nodes after");
  for(size_t i = 0; i < node_count; ++i){
    //dbg_log_c_str(all_nodes[i].node_name);
    //dbg_logint(all_nodes[i].node_id);
  }
  //dbg_log_c_str("Now finished logging after");
  //dbg_logint((int)all_nodes);
  //dbg_logint((int)(all_nodes + node_count));
  //dbg_log_c_str("Need to create node : ");
  //dbg_log_c_str(node_name);
  //dbg_logint((int)node_name);

  
  return added_node->node_id;
}

int find_node_given_ptr(BayesNode* nodes, size_t count, int node_id){
  for(size_t i = 0; i < count; ++i){
    if(nodes[i].node_id == node_id){
      return i;
    }
  }
  return -1;
}

BayesNode* find_node(int node_id){
  
  int node = find_node_given_ptr(all_nodes, node_count, node_id);
  if(node >=0)
    return all_nodes + node;
  return nullptr;
}

size_t get_parent_count(int node_id){
  BayesNode* node = find_node(node_id);
  if(node != nullptr)
    return node->parent_count;
  return 0;
}


bool insert_edge(int parent_node, int child_node){
  
  BayesNode* parent = find_node(parent_node);
  BayesNode* child = find_node(child_node);
  if((parent == nullptr) || (child == nullptr)){
    dbg_log_c_str("Either parent or child couldn't be found");
    return false;
  }

  //Check if an edge exists
  for(size_t i = 0; i < parent->parent_count; ++i){
    if(parent->parents[i] == child_node){
      dbg_log_c_str("The child is already the parent");
      return false;
    }
  }
  for(size_t i = 0; i < child->parent_count; ++i){
    if(child->parents[i] == parent_node){
      dbg_log_c_str("The parent already had that child");
      return false;
    }
  }

  //Now try realloc
  size_t parent_count = 1 + child->parent_count;
  size_t new_prob_count = ((uint)1 << ((uint)parent_count));

  int* new_parents = realloc_mem(child->parents, parent_count * sizeof * new_parents);
  if(new_parents == nullptr){
    dbg_log_c_str("Couldn't realloc parents memory");
    return false;
  }
  float* new_probs = realloc_mem(child->prob_dist, new_prob_count * sizeof * new_probs);
  if(new_probs == nullptr){
    child->parents = realloc_mem(new_parents, child->parent_count * sizeof * new_parents);
    dbg_log_c_str("Couldn't realloc prob dist memory");
    return false;
  }
  child->parents = new_parents;
  child->parent_count = parent_count;
  child->prob_dist = new_probs;
  //Initialize the parents
  new_parents[parent_count-1] = parent_node;
  for(size_t i = (new_prob_count >> 1); i < new_prob_count; ++i){
    new_probs[i] = 0.f;
  }

  need_sorting = true;
  
  return true;
}

void free_all_nodes(BayesNode* nodes, size_t count){
  for(size_t i = 0; (nodes != nullptr) && (i < count); ++i){
    if(nodes[i].prob_dist != nullptr)
      free_mem(nodes[i].prob_dist);
    if(nodes[i].parents != nullptr)
      free_mem(nodes[i].parents);
    if(nodes[i].node_name != nullptr)
      free_mem(nodes[i].node_name);
  }
  if(nodes != nullptr)
    free_mem(nodes);
}

//Make a shallow copy of the thing
BayesNode* make_shallow_copy(BayesNode* nodes, size_t count){
  if(nodes == nullptr)
    return nullptr;
  dbg_log_c_str("Inside shallow copy");
  dbg_logint((int)nodes);
  dbg_logint((int)count);
  BayesNode* copy = alloc_mem(count * sizeof * nodes);
  if(copy == nullptr)
    return nullptr;

  memcpy(copy, nodes, count * sizeof * copy);
  return copy;
}

//Make a deep copy of the thing
BayesNode* make_deep_copy(BayesNode* nodes, size_t count){

  BayesNode* copy = make_shallow_copy(nodes, count);
  if(copy == nullptr)
    return nullptr;

  bool alloc_full = true;
  for(size_t i = 0; i < count; ++i){
    size_t prob_count = ((uint)1 << ((uint)copy[i].parent_count));

    if(copy[i].node_name != nullptr){
      size_t sz = strlen(copy[i].node_name);
      char* ptr = alloc_mem(sz+1);
      if(ptr != nullptr)
	memcpy(ptr, copy[i].node_name, sz + 1);
      else
	alloc_full = false;
    }
    
    if(copy[i].parents != nullptr){
      copy[i].parents = alloc_mem(copy[i].parent_count * sizeof * copy[i].parents);
      if(copy[i].parents == nullptr)
	alloc_full = false;
      else
	memcpy(copy[i].parents, nodes[i].parents,
	       copy[i].parent_count * sizeof * copy[i].parents);
    }

    copy[i].prob_dist = alloc_mem(prob_count * sizeof * copy[i].prob_dist);
    if(copy[i].prob_dist != nullptr)
      memcpy(copy[i].prob_dist, nodes[i].prob_dist,
	     prob_count * sizeof * copy[i].prob_dist);
    else
      alloc_full = false;
  }

  if(alloc_full)
    return copy;
  free_all_nodes(nodes, count);
  return nullptr;
}

//make a shallow copy and sort
//Sorts such that each child aalways occurs before parent
bool sort_dag(BayesNode* og_nodes, size_t node_count){

  BayesNode* copy = make_shallow_copy(og_nodes, node_count);
  int* flags = alloc_mem(node_count * sizeof * flags);
  if((copy == nullptr) || (flags == nullptr))
    return false;
  memset(flags, 0,node_count * sizeof * flags);
  
  //Insert those who have no references at all
  //Insert mark if found in parent list
  for(size_t i = 0; i < node_count; ++i){
    for(size_t j = 0; j < copy[i].parent_count; ++j){
      //Find the parent id and set it to 1
      int inx = find_node_given_ptr(copy, node_count, copy[i].parents[j]);
      flags[inx] = 2;
    }
  }


  //Insert those who are 0, mark their children 1, mark them 3
  size_t inserted = 0;

  while(inserted < node_count){
    for(size_t i = 0; i < node_count; ++i){
      if(flags[i] == 0){
	og_nodes[inserted++] = copy[i];
	//Now among it's parents, unless they have
	for(size_t j = 0; j < node_count; ++j){
	  int inx = find_node_given_ptr(copy, node_count, copy[i].parents[j]);
	  if(flags[inx] == 2)
	    flags[inx] = 1;
	}
	flags[i] = 3;
      }
    }

    //Now 0 are inserted and made into 3
    //Their children if 2, made into 1

    //Now 1 are in consideration for being 0
    for(size_t i = 0; i < node_count; ++i){
      if(flags[i] == 1){
	flags[i] = 0;
      }
    }
    //Now do same thing again, but check for changing flags only if 0
    for(size_t i = 0; i < node_count; ++i){
      for(size_t j = 0; j < copy[i].parent_count; ++j){
	int inx = find_node_given_ptr(copy, node_count, copy[i].parents[j]);
	if(flags[inx] == 0)
	  flags[inx] = 2;
      }
    }
  }
  //Now nodes are inserted in sink first order
  free_mem(copy);
  free_mem(flags);
  return true;
}


typedef struct JointEntry JointEntry;
struct JointEntry {
  int desire_true;    //This bit tells if node is to bet taken true or false
  int is_required;    //This bit tells if node is processed
  int is_evidence;    //This bit tells if the node is evidence or hypothesis
};

size_t get_joint_entry_size(void){
  return sizeof(JointEntry);
}

double process_one(int node_inx, JointEntry* true_false){
  
  //For each combination of parent, find if that satisfies
  uint prob_count = all_nodes[node_inx].parent_count;
  prob_count = ((uint)1 << prob_count);

  //Find the index of the probability value required
  uint prob_inx = 0;
  for(size_t i = 0; i < all_nodes[node_inx].parent_count; i++){
    //Find if this parent is true in the list
    int inx = find_node_given_ptr(all_nodes, node_count,
				  all_nodes[node_inx].parents[i]);
    if(true_false[inx].desire_true){
      prob_inx |= ((uint)1 << i);
    }
  }
  
  //Find own probability type
  if(true_false[node_inx].desire_true)
    return all_nodes[node_inx].prob_dist[prob_inx];
  else
    return 1.0 - all_nodes[node_inx].prob_dist[prob_inx];
  
  //Now remove the probabilities of those combinations that are not in
  //joint prob list
  /* for(uint i = 0; i < prob_count; ++i){ */
  /*   //Now for each combination of i'th probability, find if that is in list */
  /*   bool matches = true; */
  /*   if(list_parents != 0) */
  /*     for(uint j = 0; j < all_nodes[node_inx].parent_count; ++j){ */

  /* 	//Find if this is involved */
  /* 	int inx = find_node_given_ptr(all_nodes, node_count, */
  /* 				      all_nodes[node_inx].parents[j]); */

  /* 	if(list[inx].is_required){ */
  /* 	  //Now if the trueness or falseness of i matches with this parent */
  /* 	  bool curr_should_true = i & ((uint)1 << j); */
  /* 	  if((curr_should_true && !list[inx].desire_true) || */
  /* 	     (!curr_should_true && list[inx].desire_true)){ */
  /* 	    matches = false; */
  /* 	    break; */
  /* 	  } */
  /* 	} */
  /*     } */
  /*   if(matches){ */
  /*     res += all_nodes[node_inx].prob_dist[i]; */
  /*   } */
  /* } */

  
  /* if(list[node_inx].desire_true) */
  /*   return res; */
  /* else */
  /*   return 1.0- res; */
}

double process_only_joint(JointEntry* entry, size_t search_start){
  //Find the first 'non required' here
  size_t first_inx = search_start;
  for(; first_inx < node_count; ++first_inx){
    if(!entry[first_inx].is_required)
      break;
  }

  if(first_inx == node_count){
    double prod = 1.0;
    for(size_t i = 0; i < node_count; ++i){
      double interm = process_one(i, entry);
      //dbg_logdouble(interm);
      prod *= interm;
    }
    return prod;
  }

  double sum = 0.0;
  entry[first_inx].is_required = 1;
  entry[first_inx].desire_true = 1;
  sum += process_only_joint(entry, 1+first_inx);
  entry[first_inx].desire_true = 0;
  sum += process_only_joint(entry, 1+first_inx);
  entry[first_inx].is_required = 0;
  return sum;
}

double process_conditional(JointEntry* entry){

  //i.e for P(H... | E...) = P(H..., E...) / P(E...)
  //First calculate P(H..., E...)
  double numerator = process_only_joint(entry, 0);
  dbg_log_c_str("Numerator value");
  dbg_logdouble(numerator);
  //Now calculate P(E...)
  //Switch the 'is_required' off for all H...
  for(size_t i = 0; i < node_count; ++i){
    if(!entry[i].is_evidence)
      entry[i].is_required = 0;
  }
  double denominator = process_only_joint(entry, 0);
  dbg_log_c_str("Denominator value");
  dbg_logdouble(denominator);
  return numerator / denominator;
}
