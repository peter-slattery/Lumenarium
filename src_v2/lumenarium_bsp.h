/* date = April 11th 2022 9:57 am */

#ifndef LUMENARIUM_BSP_H
#define LUMENARIUM_BSP_H

// NOTE(PS): Functionality Notes
// - there must always be a root node that contains the area of the tree as a whole
// - a node with no children has not been split

#define BTREE_NODE_ID_VALID_BIT (1 << 31)
struct BSP_Node_Id
{
  u32 value;
};

enum BSP_Split_Kind
{
  BSPSplit_XAxis = 1,
  BSPSplit_YAxis = 0,
  BSPSplit_ZAxis = 2,
  BSPSplit_None  = 3,
};

struct BSP_Split
{
  BSP_Split_Kind kind;
  r32 value;
};

enum BSP_Split_Update_Flags
{
  BSPSplitUpdate_None = 0,
  BSPSplitUpdate_FreeZeroAreaChildren = 1,
};

enum BSP_Child_Selector
{
  // NOTE(PS): these values are intentionally overlapping since
  // they access the data structure of the B-Tree in a particular
  // way. ie. left and top are the same space in memory as are
  // right and bottom
  BSPChild_Left   = 0,
  BSPChild_Top    = 0,
  
  BSPChild_Right  = 1,
  BSPChild_Bottom = 1,
};

struct BSP_Area
{
  v2 min;
  v2 max;
};

struct BSP_Node
{
  union
  {
    BSP_Node_Id    parent;
    BSP_Node_Id    next_free;
  };
  
  BSP_Split split;
  
  union
  {
    BSP_Node_Id children[2];
    struct
    {
      union
      {
        BSP_Node_Id left;
        BSP_Node_Id top;
      };
      union
      {
        BSP_Node_Id right;
        BSP_Node_Id bottom;
      };
    };
  };
  u32 user_data;
  
  BSP_Area area;
};

struct BSP
{
  BSP_Node* nodes;
  u32         nodes_cap;
  u32         nodes_len;
  
  BSP_Node_Id root;
  BSP_Node_Id free_first;
};

typedef void BSP_Walk_Cb(BSP* tree, BSP_Node_Id id, BSP_Node* node, u8* user_data);

internal BSP bsp_create(Allocator* allocator, u32 cap);

internal BSP_Node* bsp_get(BSP* tree, BSP_Node_Id id);
internal BSP_Node_Id bsp_push(BSP* tree, BSP_Node_Id parent, BSP_Area area, u32 user_data);
internal void bsp_free(BSP* tree, BSP_Node_Id id);
internal void bsp_free_cb(BSP* tree, BSP_Node_Id id, BSP* node, u8* user_data);

union BSP_Split_Result
{
  BSP_Node_Id children[2];
  struct
  {
    union
    {
      BSP_Node_Id left;
      BSP_Node_Id top;
    };
    union
    {
      BSP_Node_Id right;
      BSP_Node_Id bottom;
    };
  };
};

internal BSP_Split_Result  bsp_split(BSP* tree, BSP_Node_Id id, r32 split, BSP_Split_Kind kind, u32 user_data_0, u32 user_data_1);
internal void  bsp_join_recursive(BSP* tree, BSP_Node* parent, BSP_Child_Selector keep);

// left, parent, right
internal void bsp_walk_inorder(BSP* tree, BSP_Node_Id first, BSP_Walk_Cb* cb, u8* user_data);
// parent, left right
internal void bsp_walk_preorder(BSP* tree, BSP_Node_Id first, BSP_Walk_Cb* cb, u8* user_data);
// parent, right, parent
internal void bsp_walk_postorder(BSP* tree, BSP_Node_Id first, BSP_Walk_Cb* cb, u8* user_data);

internal void bsp_node_update_child_areas(BSP* tree, BSP_Node_Id id, BSP_Node* node, u8* user_data);
internal void bsp_node_area_update(BSP* tree, BSP_Node_Id id, BSP_Area new_area);
internal void bsp_child_split_update(BSP* tree, BSP_Node_Id node, u32 new_split);

///////////////////////////////////////////////////
//           IMPLEMENTATION


internal BSP
bsp_create(Allocator* allocator, u32 cap)
{
  BSP result = {};
  zero_struct(result);
  result.nodes_cap = cap;
  result.nodes = allocator_alloc_array(allocator, BSP_Node, cap);
  return result;
}

#define bsp_node_id_is_valid(id) (has_flag(id.value, BTREE_NODE_ID_VALID_BIT))
#define bsp_node_id_equals(a,b) (a.value == b.value)
#define bsp_node_id_to_index(id) (id.value & (u32)(~BTREE_NODE_ID_VALID_BIT))

internal BSP_Node*
bsp_get(BSP* tree, BSP_Node_Id id)
{
  if (!bsp_node_id_is_valid(id)) return 0;
  u32 index = bsp_node_id_to_index(id);
  if (index > tree->nodes_len) return 0;
  return tree->nodes + index;
}

internal BSP_Node_Id
bsp_push(BSP* tree, BSP_Node_Id parent_id, BSP_Area area, u32 user_data)
{
  BSP_Node_Id result = BSP_Node_Id{0};
  BSP_Node* node = 0;
  
  if (tree->nodes_len >= tree->nodes_cap) 
  {
    if (bsp_node_id_is_valid(tree->free_first))
    {
      result = tree->free_first;
      node = bsp_get(tree, result);
      tree->free_first = node->next_free;
      zero_struct(node->parent);
    }
  }
  else
  {
    result.value = tree->nodes_len++;
    assert(!has_flag(result.value, BTREE_NODE_ID_VALID_BIT));
    add_flag(result.value, BTREE_NODE_ID_VALID_BIT);
    node = tree->nodes + bsp_node_id_to_index(result);
  }
  
  if (bsp_node_id_is_valid(result))
  {
    node->split.kind = BSPSplit_None;
    node->parent = parent_id;
    node->area = area;
    node->user_data = user_data;
  }
  
  return result;
}

internal void
bsp_free_(BSP* tree, BSP_Node_Id id, BSP_Node* now_free)
{
  if (bsp_node_id_is_valid(now_free->parent))
  {
    BSP_Node* parent = bsp_get(tree, now_free->parent);
    if (bsp_node_id_equals(parent->children[0], id))
    {
      zero_struct(parent->children[0]);
    }
    else if (bsp_node_id_equals(parent->children[1], id))
    {
      zero_struct(parent->children[1]);
    }
    else
    {
      // NOTE(PS): in this case, a child node had a reference to
      // a parent that didn't have a reference back to the child
      // this means the tree itself is messed up
      invalid_code_path;
    }
  }
  
  zero_struct(*now_free);
  now_free->next_free = tree->free_first;
  tree->free_first = id;
}

internal void
bsp_free(BSP* tree, BSP_Node_Id id)
{
  BSP_Node* now_free = bsp_get(tree, id);
  bsp_free_(tree, id, now_free);
}

internal void
bsp_free_cb(BSP* tree, BSP_Node_Id id, BSP_Node* node, u8* user_data)
{
  bsp_free_(tree, id, node);
}

internal BSP_Split_Result
bsp_split(BSP* tree, BSP_Node_Id node_id, r32 split, BSP_Split_Kind kind, u32 user_data_0, u32 user_data_1)
{
  BSP_Node* node = bsp_get(tree, node_id);
  split = clamp(node->area.min.Elements[kind], split, node->area.max.Elements[kind]);
  node->split.value = split;
  node->split.kind = kind;
  node->children[0] = bsp_push(tree, node_id, {}, user_data_0);
  node->children[1] = bsp_push(tree, node_id, {}, user_data_1);
  bsp_node_update_child_areas(tree, node_id, node, 0);
  
  BSP_Split_Result result = {};
  result.children[0] = node->children[0];
  result.children[1] = node->children[1];
  return result;
}

internal void
bsp_join_recursive(BSP* tree, BSP_Node_Id parent_id, BSP_Child_Selector keep)
{
  BSP_Node* parent = bsp_get(tree, parent_id);
  BSP_Node keep_node = *bsp_get(tree, parent->children[keep]);
  bsp_walk_preorder(tree, parent->children[0], bsp_free_cb, 0);
  bsp_walk_preorder(tree, parent->children[1], bsp_free_cb, 0);
  parent->user_data = keep_node.user_data;
  zero_struct(parent->children[0]);
  zero_struct(parent->children[1]);
}

// NOTE(PS): the other three walk functions all require allocation of a stack
// while this is fast with our scratch allocator, there are cases where, for
// correctness, we walk a tree that is very likely to be a single node. In 
// those cases, we can avoid allocating anything by just visiting the single
// node and returning early.
// This function provides that functionality for all walk functions
internal bool
bsp_walk_single_node_check(BSP* tree, BSP_Node_Id first, BSP_Walk_Cb* visit, u8* user_data)
{
  BSP_Node* node = bsp_get(tree, first);
  if (node->split.kind == BSPSplit_None)
  {
    visit(tree, first, node, user_data);
    return true;
  }
  return false;
}

// left, parent, right
internal void 
bsp_walk_inorder(BSP* tree, BSP_Node_Id first, BSP_Walk_Cb* visit, u8* user_data)
{
  if (!bsp_node_id_is_valid(first)) return;
  if (bsp_walk_single_node_check(tree, first, visit, user_data)) return;
  scratch_get(scratch);
  BSP_Node_Id* stack = allocator_alloc_array(scratch.a, BSP_Node_Id, tree->nodes_len);
  u32 stack_len = 0;
  memory_zero_array(stack, BSP_Node_Id, tree->nodes_len);
  
  BSP_Node_Id at = first;
  while (true)
  {
    if (bsp_node_id_is_valid(at))
    {
      stack[stack_len++] = at;
      BSP_Node* n = bsp_get(tree, at);
      at = n->children[0];
    }
    else
    {
      if (stack_len == 0) break;
      
      at = stack[--stack_len];
      BSP_Node* n = bsp_get(tree, at);
      visit(tree, at, n, user_data);
      at = n->children[1];
    }
  }
}

// parent, left right
internal void
bsp_walk_preorder(BSP* tree, BSP_Node_Id first, BSP_Walk_Cb* visit, u8* user_data)
{
  if (bsp_walk_single_node_check(tree, first, visit, user_data)) return;
  scratch_get(scratch);
  BSP_Node_Id* stack = allocator_alloc_array(scratch.a, BSP_Node_Id, tree->nodes_len);
  u32 stack_len = 0;
  memory_zero_array(stack, BSP_Node_Id, tree->nodes_len);
  
  BSP_Node_Id at = first;
  
  while (true)
  {
    while (bsp_node_id_is_valid(at))
    {
      BSP_Node* n = bsp_get(tree, at);
      visit(tree, at, n, user_data);
      stack[stack_len++] = at;
      at = n->children[0];
    }
    
    if (!bsp_node_id_is_valid(at) && stack_len == 0) break;
    
    at = stack[--stack_len];
    BSP_Node* n = bsp_get(tree, at);
    at = n->children[1];
  }
}

// parent, right, parent
internal void
bsp_walk_postorder(BSP* tree, BSP_Node_Id first, BSP_Walk_Cb* visit, u8* user_data)
{
  if (bsp_walk_single_node_check(tree, first, visit, user_data)) return;
  scratch_get(scratch);
  BSP_Node_Id* stack = allocator_alloc_array(scratch.a, BSP_Node_Id, tree->nodes_len);
  u32 stack_len = 0;
  memory_zero_array(stack, BSP_Node_Id, tree->nodes_len);
  
  BSP_Node_Id at = first;
  while (true)
  {
    if (bsp_node_id_is_valid(at))
    {
      BSP_Node* n = bsp_get(tree, at);
      if (bsp_node_id_is_valid(n->children[1])) stack[stack_len++] = n->children[1];
      stack[stack_len++] = at;
      at = n->children[0];
    }
    else
    {
      if (stack_len == 0) break;
      
      at = stack[--stack_len];
      BSP_Node* n = bsp_get(tree, at);
      assert(n != 0);
      
      if (bsp_node_id_is_valid(n->children[1]) && bsp_node_id_equals(n->children[1], stack[stack_len - 1]))
      {
        BSP_Node_Id at_temp = stack[stack_len - 1];
        stack[stack_len - 1] = at;
        at = at_temp;
      }
      else
      {
        visit(tree, at, n, user_data);
        zero_struct(at);
      }
    }
  }
}

internal void
bsp_node_update_child_areas(BSP* tree, BSP_Node_Id id, BSP_Node* node, u8* user_data)
{
  // assume that node's area is correct. Using that, clamp its split as appropriate
  // and then update its children. If a child has an area of zero, rely on flags
  // to determine behavior
  
  if (node->split.kind == BSPSplit_None) return;
  
  BSP_Split_Update_Flags flags = (BSP_Split_Update_Flags)0;
  if (user_data) flags = *(BSP_Split_Update_Flags*)user_data;
  
  BSP_Split_Kind kind = node->split.kind;
  
  BSP_Node* node_min = bsp_get(tree, node->children[0]);
  BSP_Node* node_max = bsp_get(tree, node->children[1]);
  node_min->area = node->area;
  node_max->area = node->area;
  node_min->area.max.Elements[kind] = node->split.value;
  node_max->area.min.Elements[kind] = node->split.value;
  
  if (has_flag(flags, BSPSplitUpdate_FreeZeroAreaChildren))
  {
    bool free_children = false;
    if (node_min->area.max.Elements[kind] <= node_min->area.min.Elements[kind])
    {
      node->user_data = node_max->user_data;
      free_children= true;
    }
    else if (node_max->area.max.Elements[kind] <= node_max->area.min.Elements[kind])
    {
      node->user_data = node_min->user_data;
      free_children= true;
    }
    
    if (free_children)
    {
      bsp_walk_postorder(tree, node->children[0], bsp_free_cb, 0);
      bsp_walk_postorder(tree, node->children[1], bsp_free_cb, 0);
    }
    
  }
  
  // NOTE(PS): no need to recurse, this function assumes its either being
  // called on a particular node or its the callback of one of the tree
  // walk functions
}

internal void
bsp_node_area_update(BSP* tree, BSP_Node_Id node_id, BSP_Area area)
{
  BSP_Node* node = bsp_get(tree, node_id);
  node->area = area;
  BSP_Split_Update_Flags flags = BSPSplitUpdate_FreeZeroAreaChildren;
  bsp_walk_preorder(tree, node_id, bsp_node_update_child_areas, (u8*)&flags);
}

internal void
bsp_child_split_update(BSP* tree, BSP_Node_Id node_id, r32 new_split, BSP_Split_Update_Flags flags)
{
  BSP_Node* node = bsp_get(tree, node_id);
  node->split.value = new_split;
  bsp_walk_preorder(tree, node_id, bsp_node_update_child_areas, (u8*)&flags);
}

#if defined(DEBUG)

internal void
bsp_tests()
{
  scratch_get(scratch);
  BSP tree = bsp_create(scratch.a, 256);
  tree.root = bsp_push(&tree, {0}, {{0,0},{512,512}}, 0);
  
  BSP_Split_Result r0 = bsp_split(&tree, tree.root, 256, BSPSplit_YAxis, 0, 0);
  BSP_Node* root = bsp_get(&tree, tree.root);
  BSP_Node* n0 = bsp_get(&tree, r0.children[0]);
  BSP_Node* n1 = bsp_get(&tree, r0.children[1]);
  assert(root != 0 && n0 != 0 && n1 != 0);
  assert(n0->area.min == root->area.min);
  assert(n0->area.max.x == 256 && n0->area.max.y == root->area.max.y);
  assert(n1->area.max == root->area.max);
  assert(n1->area.min.x == 256 && n0->area.min.y == root->area.min.y);
  assert(n0->split.kind == BSPSplit_None);
  assert(n1->split.kind == BSPSplit_None);
  assert(root->split.kind == BSPSplit_YAxis);
  
  BSP_Split_Result r1 = bsp_split(&tree, root->children[0], 32, BSPSplit_YAxis, 0, 0);
  BSP_Split_Result r2 = bsp_split(&tree, r1.children[1], 64, BSPSplit_XAxis, 0, 0);
  
  bsp_walk_postorder(&tree, root->children[0], bsp_free_cb, 0);
}

#else

#define bsp_tests() 

#endif

#endif //LUMENARIUM_BSP_H
