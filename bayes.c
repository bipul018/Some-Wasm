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



static BayesNode* nodes = nullptr;
static size_t node_count = 0;

//-1 implies failure to create new node
int create_new_node(const char* node_name){
  //dbg_log_c_str("Need to create node : ");
  //dbg_log_c_str(node_name);
  //dbg_logint((int)node_name);
  //dbg_log_c_str("Logging all the nodes first");
  //dbg_logint((int)nodes);
  //dbg_logint((int)(nodes + node_count));
  //dbg_log_c_str("nodes");
  for(size_t i = 0; i < node_count; ++i){
    //dbg_log_c_str(nodes[i].node_name);
    //dbg_logint(nodes[i].node_id);
  }
  //dbg_log_c_str("Now finished logging previous");
    
  BayesNode* added_node = realloc_mem(nodes,
				      (node_count + 1) *
				      sizeof(BayesNode));
  if(added_node == nullptr)
    return -1;
  if(added_node != nodes){
    //dbg_log_c_str("Pointer changed");
  }
  nodes = added_node;

  added_node = nodes + node_count;
  node_count++;

  added_node->node_id = node_count - 1;
  added_node->node_name = node_name;
  added_node->parent_count = 0;
  added_node->prob_dist = nullptr;
  added_node->parents = nullptr;

  
  //dbg_log_c_str("Logging all the nodes after");
  for(size_t i = 0; i < node_count; ++i){
    //dbg_log_c_str(nodes[i].node_name);
    //dbg_logint(nodes[i].node_id);
  }
  //dbg_log_c_str("Now finished logging after");
  //dbg_logint((int)nodes);
  //dbg_logint((int)(nodes + node_count));
  //dbg_log_c_str("Need to create node : ");
  //dbg_log_c_str(node_name);
  //dbg_logint((int)node_name);

  
  return added_node->node_id;
}

BayesNode* find_node(int node_id){
  //dbg_log_c_str("In find node, ");
  //dbg_logint(node_id);
  for(size_t i = 0; i < node_count; ++i){
    if(nodes[i].node_id == node_id){
      //dbg_log_c_str("Found node, returning ptr ");
      //dbg_logint((int)(nodes + i));
      return nodes + i;
    }
  }
  //  dbg_log_c_str("Not found any node");
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
  return true;
}
