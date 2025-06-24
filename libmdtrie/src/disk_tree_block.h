#ifndef DISK_MD_TRIE_TREE_BLOCK_H
#define DISK_MD_TRIE_TREE_BLOCK_H

#include "disk_compact_ptr.h"
#include "disk_compressed_bitmap.h"
#include "point_array.h"
#include "disk_trie_node.h"
#include <cmath>
#include <sys/time.h>

template <dimension_t DIMENSION>
class disk_tree_block
{
public:
  // deleted constructors

  inline preorder_t num_frontiers() { return num_frontiers_; }

  inline disk_tree_block *get_pointer(preorder_t current_frontier, uint64_t base)
  {

    disk_frontier_node<DIMENSION> *front_ptrs = (disk_frontier_node<DIMENSION> *)((uint64_t)frontiers_ + base);
    return (disk_tree_block *)((uint64_t)(front_ptrs[current_frontier].pointer_) + base);
  }

  inline preorder_t get_preorder(preorder_t current_frontier, uint64_t base)
  {
    disk_frontier_node<DIMENSION> *front_ptrs = (disk_frontier_node<DIMENSION> *)((uint64_t)frontiers_ + base);
    return front_ptrs[current_frontier].preorder_;
  }

  // deleted: mutators

  std::vector<disk_bits::disk_compact_ptr> get_primary_key_list()
  {
    // todo: this should be a disk vector
    return primary_key_list;
  }

  preorder_t select_subtree(preorder_t &subtree_size,
                            preorder_t &selected_node_depth,
                            preorder_t &selected_node_pos,
                            preorder_t &num_primary,
                            preorder_t &selected_primary_index,
                            preorder_t *node_to_primary,
                            preorder_t *node_to_depth, uint64_t base)
  {

    // index -> Number of children & preorder
    node_info index_to_node[4096];

    // index -> size of subtree & preorder
    subtree_info index_to_subtree[4096];
    num_primary = 0;

    // Index -> depth of the node
    preorder_t index_to_depth[4096];

    //  Corresponds to index_to_node, index_to_subtree, index_to_depth
    preorder_t node_stack_top = 0, subtree_stack_top = 0, depth_stack_top = 0;
    preorder_t current_frontier = 0;
    selected_primary_index = 0;

    node_pos_t current_node_pos = 0;
    index_to_node[node_stack_top].preorder_ = 0;
    index_to_node[node_stack_top].n_children_ =
        ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))->get_num_children(0, 0, level_to_num_children[root_depth_], base);

    node_stack_top++;
    node_to_depth[0] = root_depth_;
    preorder_t prev_depth = root_depth_;
    level_t depth = root_depth_ + 1;
    preorder_t next_frontier_preorder;

    if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
      next_frontier_preorder = -1;
    else
      next_frontier_preorder = get_preorder(current_frontier, base);

    for (preorder_t i = 1; i < num_nodes_; i++)
    {

      current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                              ->get_num_bits(i - 1, prev_depth, base);
      prev_depth = depth;

      node_to_depth[i] = depth;
      if (depth == max_depth_ - 1)
      {

        node_to_primary[i] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                 ->get_num_children(i, current_node_pos, level_to_num_children[depth], base);
      }
      if (i == next_frontier_preorder)
      {

        current_frontier++;
        if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
          next_frontier_preorder = -1;
        else
          next_frontier_preorder = get_preorder(current_frontier, base);
        index_to_node[node_stack_top - 1].n_children_--;
      }
      //  Start searching for its children
      else if (depth < max_depth_ - 1)
      {

        index_to_node[node_stack_top].preorder_ = i;
        index_to_node[node_stack_top++].n_children_ = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                                          ->get_num_children(i, current_node_pos, level_to_num_children[depth], base);
        depth++;
      }
      //  Reached the maxDepth level
      else
      {
        index_to_node[node_stack_top - 1].n_children_--;
      }

      while (node_stack_top > 0 &&
             index_to_node[node_stack_top - 1].n_children_ == 0)
      {

        index_to_subtree[subtree_stack_top].preorder_ =
            index_to_node[node_stack_top - 1].preorder_;
        index_to_subtree[subtree_stack_top].subtree_size_ =
            i - index_to_node[node_stack_top - 1].preorder_ + 1;
        subtree_stack_top++;
        node_stack_top--;
        index_to_depth[depth_stack_top] = depth;
        depth_stack_top++;
        depth--;
        if (node_stack_top == 0)
          break;
        else
          index_to_node[node_stack_top - 1].n_children_--;
      }
    }
    current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                            ->get_num_bits(num_nodes_ - 1, prev_depth, base);

    // Go through the index_to_subtree vector to choose the proper subtree
    preorder_t min_node = 0;
    preorder_t min = (preorder_t)-1;
    preorder_t min_index = 0;
    preorder_t diff = (preorder_t)-1;
    auto leftmost = (preorder_t)-1;

    for (preorder_t i = 0; i < subtree_stack_top; i++)
    {
      auto subtree_size_at_i = (preorder_t)index_to_subtree[i].subtree_size_;
      if (index_to_subtree[i].preorder_ != 0 &&
          num_nodes_ <= subtree_size_at_i * 4 &&
          subtree_size_at_i * 4 <= 3 * num_nodes_ &&
          index_to_subtree[i].preorder_ < leftmost)
      {

        leftmost = min_node = index_to_subtree[i].preorder_;
        min_index = i;
      }
    }

    if (leftmost == (preorder_t)-1)
    {
      min_node = index_to_subtree[1].preorder_;
      if (num_nodes_ > 2 * index_to_subtree[1].subtree_size_)
      {
        min = num_nodes_ - 2 * index_to_subtree[1].subtree_size_;
      }
      else
      {
        min = 2 * index_to_subtree[1].subtree_size_ - num_nodes_;
      }
      min_index = 1;

      for (preorder_t i = 1; i < subtree_stack_top; i++)
      {
        if (num_nodes_ > 2 * index_to_subtree[i].subtree_size_)
        {
          diff = num_nodes_ - 2 * index_to_subtree[i].subtree_size_;
        }
        else
        {
          diff = 2 * index_to_subtree[i].subtree_size_ - num_nodes_;
        }
        if (diff < min)
        {
          min = diff;
          min_node = index_to_subtree[i].preorder_;
          min_index = i;
        }
      }
    }
    subtree_size = index_to_subtree[min_index].subtree_size_;
    selected_node_depth = index_to_depth[min_index];

    for (preorder_t i = 0; i < min_node; i++)
    {
      selected_primary_index += node_to_primary[i];
    }
    for (preorder_t i = 1; i <= min_node; i++)
    {
      selected_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                               ->get_num_bits(i - 1, node_to_depth[i - 1], base);
    }
    for (preorder_t i = min_node; i < min_node + subtree_size; i++)
    {
      num_primary += node_to_primary[i];
    }
    selected_node_depth = node_to_depth[min_node];

    return min_node;
  }

  // This function takes in a node (in preorder) and a symbol (branch index)
  // Return the child node (in preorder) designated by that symbol
  preorder_t skip_children_subtree(preorder_t node,
                                   preorder_t &node_pos,
                                   morton_t symbol,
                                   level_t current_level,
                                   preorder_t &current_frontier,
                                   preorder_t &current_primary,
                                   uint64_t base)
  {

    if (current_level == max_depth_)
    {
      return node;
    }

    int sTop = -1;
    preorder_t n_children_skip = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                     ->get_child_skip(node, node_pos, symbol, level_to_num_children[current_level], base);
    preorder_t n_children = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))->get_num_children(node, node_pos, level_to_num_children[current_level], base);
    preorder_t diff = n_children - n_children_skip;
    preorder_t stack[100];
    sTop++;
    stack[sTop] = n_children;

    preorder_t current_node_pos =
        node_pos + ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                       ->get_num_bits(node, current_level, base); // TODO
    preorder_t current_node = node + 1;

    if (frontiers_ != nullptr && current_frontier < num_frontiers_ &&
        current_node > get_preorder(current_frontier, base))
      ++current_frontier;
    preorder_t next_frontier_preorder;

    if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
      next_frontier_preorder = -1;
    else
      next_frontier_preorder = get_preorder(current_frontier, base);

    current_level++;
    while (current_node < num_nodes_ && sTop >= 0 && diff < stack[0])
    {
      if (current_node == next_frontier_preorder)
      {
        current_frontier++;
        if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
          next_frontier_preorder = -1;
        else
          next_frontier_preorder = get_preorder(current_frontier, base);
        stack[sTop]--;

        current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                ->get_num_bits(current_node, current_level, base);
      }
      // It is "-1" because current_level is 0th indexed.
      else if (current_level < max_depth_ - 1)
      {
        sTop++;
        stack[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                          ->get_num_children(current_node, current_node_pos, level_to_num_children[current_level], base);

        current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                ->get_num_bits(current_node, current_level, base);
        current_level++;
      }
      else
      {
        stack[sTop]--;

        if (current_level == max_depth_ - 1)
        {

          current_primary +=
              ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                  ->get_num_children(current_node,
                                     current_node_pos,
                                     level_to_num_children[current_level], base);
        }
        current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                ->get_num_bits(current_node, current_level, base);
      }
      current_node++;

      while (sTop >= 0 && stack[sTop] == 0)
      {
        sTop--;
        current_level--;
        if (sTop >= 0)
          stack[sTop]--;
      }
    }
    node_pos = current_node_pos;
    return current_node;
  }

  // This function takes in a node (in preorder) and a symbol (branch index)
  // Return the child node (in preorder) designated by that symbol
  preorder_t skip_children_subtree_range_search(
      preorder_t node,
      preorder_t &node_pos,
      morton_t symbol,
      level_t current_level,
      preorder_t &current_frontier,
      preorder_t &current_primary,
      preorder_t stack[100],
      int &sTop,
      preorder_t &current_node_pos,
      preorder_t &current_node,
      preorder_t &next_frontier_preorder,
      preorder_t &current_frontier_cont,
      preorder_t &current_primary_cont,
      uint64_t base)
  {

    if (current_level == max_depth_)
    {
      return node;
    }

    preorder_t n_children_skip = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                     ->get_child_skip(
                                         node, node_pos, symbol, level_to_num_children[current_level], base);
    preorder_t n_children = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                ->get_num_children(
                                    node, node_pos, level_to_num_children[current_level], base);
    preorder_t diff = n_children - n_children_skip;

    bool first_time = false;
    if (sTop == -1)
    {
      sTop++;
      stack[sTop] = n_children;
      first_time = true;
    }

    if (first_time)
      current_node_pos =
          node_pos + ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                         ->get_num_bits(node, current_level, base); // TODO
    if (first_time)
      current_node = node + 1;

    if (first_time)
    {
      if (frontiers_ != nullptr && current_frontier < num_frontiers_ &&
          current_node > get_preorder(current_frontier, base))
        ++current_frontier;
    }
    else
    {
      current_frontier = current_frontier_cont;
      current_primary = current_primary_cont;
    }

    if (first_time)
    {
      if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
        next_frontier_preorder = -1;
      else
        next_frontier_preorder = get_preorder(current_frontier, base);
    }

    current_level++;

    while ((current_node < num_nodes_ && sTop >= 0 && diff < stack[0]) ||
           !first_time)
    {

      // First time needs to go down first.
      first_time = true;
      if (current_node == next_frontier_preorder)
      {
        current_frontier++;
        if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
          next_frontier_preorder = -1;
        else
          next_frontier_preorder = get_preorder(current_frontier, base);
        stack[sTop]--;

        current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                ->get_num_bits(current_node, current_level, base);
      }
      // It is "-1" because current_level is 0th indexed.
      else if (current_level < max_depth_ - 1)
      {
        sTop++;
        stack[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                          ->get_num_children(
                              current_node, current_node_pos, level_to_num_children[current_level], base);

        current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                ->get_num_bits(current_node, current_level, base);
        current_level++;
      }
      else
      {
        stack[sTop]--;

        if (current_level == max_depth_ - 1)
        {

          current_primary +=
              ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                  ->get_num_children(current_node,
                                     current_node_pos,
                                     level_to_num_children[current_level], base);
        }
        current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                ->get_num_bits(current_node, current_level, base);
      }
      current_node++;

      while (sTop >= 0 && stack[sTop] == 0)
      {
        sTop--;
        current_level--;
        if (sTop >= 0)
          stack[sTop]--;
      }
    }
    node_pos = current_node_pos;
    current_frontier_cont = current_frontier;
    current_primary_cont = current_primary;
    return current_node;
  }

  // This function takes in a node (in preorder) and a symbol (branch index)
  // Return the child node (in preorder) designated by that symbol
  // This function differs from skip_children_subtree as it checks if that child
  // node is present
  preorder_t child(disk_tree_block<DIMENSION> *&p,
                   preorder_t node,
                   preorder_t &node_pos,
                   morton_t symbol,
                   level_t current_level,
                   preorder_t &current_frontier,
                   preorder_t &current_primary,
                   uint64_t base)
  {

    if (node >= num_nodes_)
      return null_node;

    auto has_child = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                         ->has_symbol(
                             node, node_pos, symbol, level_to_num_children[current_level], base);
    if (!has_child)
      return null_node;

    if (current_level == max_depth_ - 1)
      return node;

    preorder_t current_node;

    if (frontiers_ != nullptr && current_frontier < num_frontiers_ &&
        node == get_preorder(current_frontier, base))
    {

      p = get_pointer(current_frontier, base);
      current_frontier = 0;
      current_primary = 0;
      preorder_t temp_node = 0;
      preorder_t temp_node_pos = 0;
      current_node = p->skip_children_subtree(temp_node,
                                              temp_node_pos,
                                              symbol,
                                              current_level,
                                              current_frontier,
                                              current_primary, base);
      node_pos = temp_node_pos;
    }
    else
    {
      current_node = skip_children_subtree(node,
                                           node_pos,
                                           symbol,
                                           current_level,
                                           current_frontier,
                                           current_primary, base);
    }
    return current_node;
  }

  // This function takes in a node (in preorder) and a symbol (branch index)
  // Return the child node (in preorder) designated by that symbol
  // This function differs from skip_children_subtree as it checks if that child
  // node is present
  preorder_t child_range_search(preorder_t node,
                                preorder_t &node_pos,
                                morton_t symbol,
                                level_t current_level,
                                preorder_t &current_frontier,
                                preorder_t &current_primary,
                                preorder_t stack[100],
                                int &sTop,
                                preorder_t &current_node_pos,
                                preorder_t &current_node,
                                preorder_t &next_frontier_preorder,
                                preorder_t &current_frontier_cont,
                                preorder_t &current_primary_cont,
                                uint64_t base)
  {

    if (node >= num_nodes_)
      return null_node;

    auto has_child = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                         ->has_symbol(
                             node, node_pos, symbol, level_to_num_children[current_level], base);
    if (!has_child)
      return null_node;

    if (current_level == max_depth_ - 1)
      return node;

    preorder_t current_node_ret;

    current_node_ret =
        skip_children_subtree_range_search(node,
                                           node_pos,
                                           symbol,
                                           current_level,
                                           current_frontier,
                                           current_primary,
                                           stack,
                                           sTop,
                                           current_node_pos,
                                           current_node,
                                           next_frontier_preorder,
                                           current_frontier_cont,
                                           current_primary_cont,
                                           base);
    return current_node_ret;
  }

  // This function is used for testing.
  // It differs from above as it only returns True or False.
  bool walk_tree_block(data_point<DIMENSION> *leaf_point, level_t level, uint64_t base)
  {

    preorder_t current_frontier = 0;
    preorder_t current_primary = 0;
    preorder_t current_node = 0;
    preorder_t temp_node = 0;
    preorder_t temp_node_pos = 0;

    while (level < max_depth_)
    {
      morton_t current_symbol = leaf_point->leaf_to_symbol(level);

      tree_block<DIMENSION> *current_treeblock = this;
      temp_node = child(current_treeblock,
                        current_node,
                        temp_node_pos,
                        current_symbol,
                        level,
                        current_frontier,
                        current_primary, base);

      if (temp_node == (preorder_t)-1)
      {
        return false;
      }
      current_node = temp_node;

      if (num_frontiers() > 0 && current_frontier < num_frontiers() &&
          current_node == get_preorder(current_frontier, base))
      {
        disk_tree_block<DIMENSION> *next_block = get_pointer(current_frontier, base);

        return next_block->walk_tree_block(leaf_point, level + 1, base);
      }
      level++;
    }
    return true;
  }

  void get_node_path(preorder_t node, std::vector<morton_t> &node_path, uint64_t base)
  {

    if (node == 0)
    {
      node_path[root_depth_] =
          ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
              ->next_symbol(0,
                            0,
                            0,
                            (1 << level_to_num_children[root_depth_]) - 1,
                            level_to_num_children[root_depth_], base);
      if (root_depth_ != trie_depth_)
      {
        ((disk_tree_block<DIMENSION> *)((uint64_t)disk_parent_combined_ptr_ + base))
            ->get_node_path(treeblock_frontier_num_, node_path, base);
      }
      else
      {
        ((disk_trie_node<DIMENSION> *)((uint64_t)disk_parent_combined_ptr_ + base))
            ->get_node_path(root_depth_, node_path, base);
      }
      return;
    }

    preorder_t stack[64] = {};
    preorder_t path[64] = {};
    uint64_t symbol[64];
    level_t sTop_to_level[64] = {};

    for (uint16_t i = 0; i < 64; i++)
    {
      symbol[i] = -1;
    }
    size_t node_positions[2048];
    node_positions[0] = 0;
    preorder_t current_frontier = 0;
    int sTop = 0;

    preorder_t top_node = 0;
    preorder_t top_node_pos = 0;

    symbol[sTop] =
        ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
            ->next_symbol(symbol[sTop] + 1,
                          top_node,
                          top_node_pos,
                          (1 << level_to_num_children[root_depth_]) - 1,
                          level_to_num_children[root_depth_],
                          base);

    stack[sTop] =
        ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
            ->get_num_children(0, 0, level_to_num_children[root_depth_], base);
    sTop_to_level[sTop] = root_depth_;

    level_t current_level = root_depth_ + 1;
    preorder_t current_node = 1;
    preorder_t current_node_pos = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                      ->get_num_bits(0, root_depth_, base);

    if (frontiers_ != nullptr && current_frontier < num_frontiers_ &&
        current_node > get_preorder(current_frontier, base))
      ++current_frontier;
    preorder_t next_frontier_preorder;

    if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
      next_frontier_preorder = -1;
    else
      next_frontier_preorder = get_preorder(current_frontier, base);

    while (current_node < num_nodes_ && sTop >= 0)
    {

      node_positions[current_node] = current_node_pos;
      current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                              ->get_num_bits(current_node, current_level, base);

      if (current_node == next_frontier_preorder)
      {
        if (current_node != node)
        {
          top_node = path[sTop];
          symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                             ->next_symbol(
                                 symbol[sTop] + 1,
                                 top_node,
                                 node_positions[top_node],
                                 (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                                 level_to_num_children[sTop_to_level[sTop]],
                                 base);
        }
        ++current_frontier;
        if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
          next_frontier_preorder = -1;
        else
          next_frontier_preorder = get_preorder(current_frontier, base);

        --stack[sTop];
      }
      // It is "-1" because current_level is 0th indexed.
      else if (current_level < max_depth_ - 1)
      {
        sTop++;
        stack[sTop] =
            ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                ->get_num_children(current_node,
                                   node_positions[current_node],
                                   level_to_num_children[current_level], base);
        path[sTop] = current_node;
        sTop_to_level[sTop] = current_level;

        symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                           ->next_symbol(
                               symbol[sTop] + 1,
                               current_node,
                               node_positions[current_node],
                               (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                               level_to_num_children[sTop_to_level[sTop]],
                               base);
        ++current_level;
      }
      else if (current_level == max_depth_ - 1 && stack[sTop] > 1 &&
               current_node < node)
      {
        top_node = path[sTop];
        symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                           ->next_symbol(
                               symbol[sTop] + 1,
                               top_node,
                               node_positions[top_node],
                               (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                               level_to_num_children[sTop_to_level[sTop]],
                               base);
        --stack[sTop];
      }
      else
      {
        --stack[sTop];
      }

      if (current_node == node)
      {
        break;
      }

      ++current_node;
      bool backtracekd = false;
      while (sTop >= 0 && stack[sTop] == 0)
      {
        backtracekd = true;
        path[sTop] = 0;
        symbol[sTop] = -1;
        --sTop;
        --current_level;
        if (sTop >= 0)
          --stack[sTop];
      }
      if (backtracekd)
      {
        top_node = path[sTop];
        symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                           ->next_symbol(
                               symbol[sTop] + 1,
                               top_node,
                               node_positions[top_node],
                               (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                               level_to_num_children[sTop_to_level[sTop]],
                               base);
      }
    }
    if (current_node == num_nodes_)
    {
      fprintf(stderr, "node not found!\n");
      return;
    }
    for (int i = 0; i <= sTop; i++)
    {
      node_path[root_depth_ + i] = symbol[i];
    }
    if (root_depth_ != trie_depth_)
    {

      ((disk_tree_block<DIMENSION> *)((uint64_t)disk_parent_combined_ptr_ + base))
          ->get_node_path(treeblock_frontier_num_, node_path, base);
    }
    else
    {
      ((disk_trie_node<DIMENSION> *)((uint64_t)disk_parent_combined_ptr_ + base))
          ->get_node_path(root_depth_, node_path, base);
    }
  }

  morton_t get_node_path_primary_key(n_leaves_t primary_key,
                                     std::vector<morton_t> &node_path, uint64_t base)
  {

    preorder_t stack[64] = {};
    preorder_t path[64] = {};
    int symbol[64];
    level_t sTop_to_level[64] = {};

    for (uint16_t i = 0; i < 64; i++)
    {
      symbol[i] = -1;
    }
    size_t node_positions[2048];
    node_positions[0] = 0;
    int sTop = 0;
    preorder_t top_node = 0;
    symbol[sTop] =
        ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
            ->next_symbol(symbol[sTop] + 1,
                          0,
                          0,
                          (1 << level_to_num_children[root_depth_]) - 1,
                          level_to_num_children[root_depth_],
                          base);
    stack[sTop] =
        ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
            ->get_num_children(0, 0, level_to_num_children[root_depth_], base);
    sTop_to_level[sTop] = root_depth_;

    level_t current_level = root_depth_ + 1;
    preorder_t current_node = 1;
    preorder_t current_node_pos = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                      ->get_num_bits(0, root_depth_, base);

    preorder_t current_frontier = 0;
    preorder_t current_primary = 0;

    if (frontiers_ != nullptr && current_frontier < num_frontiers_ &&
        current_node > get_preorder(current_frontier, base))
      ++current_frontier;
    preorder_t next_frontier_preorder;
    morton_t parent_symbol = -1;
    if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
      next_frontier_preorder = -1;
    else
      next_frontier_preorder = get_preorder(current_frontier, base);

    while (current_node < num_nodes_ && sTop >= 0)
    {

      node_positions[current_node] = current_node_pos;
      current_node_pos += ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                              ->get_num_bits(current_node, current_level, base);

      if (current_node == next_frontier_preorder)
      {
        top_node = path[sTop];
        symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                           ->next_symbol(
                               symbol[sTop] + 1,
                               top_node,
                               node_positions[top_node],
                               (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                               level_to_num_children[sTop_to_level[sTop]], base);
        ++current_frontier;
        if (num_frontiers_ == 0 || current_frontier >= num_frontiers_)
          next_frontier_preorder = -1;
        else
          next_frontier_preorder = get_preorder(current_frontier, base);

        --stack[sTop];
      }
      // It is "-1" because current_level is 0th indexed.
      else if (current_level < max_depth_ - 1)
      {
        sTop++;
        stack[sTop] =
            ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                ->get_num_children(current_node,
                                   node_positions[current_node],
                                   level_to_num_children[current_level], base);
        path[sTop] = current_node;
        sTop_to_level[sTop] = current_level;

        symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                           ->next_symbol(
                               symbol[sTop] + 1,
                               current_node,
                               node_positions[current_node],
                               (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                               level_to_num_children[sTop_to_level[sTop]],
                               base);
        ++current_level;
      }
      else
      {
        --stack[sTop];
        if (current_level == max_depth_ - 1)
        {

          preorder_t new_current_primary =
              current_primary +
              ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                  ->get_num_children(current_node,
                                     node_positions[current_node],
                                     level_to_num_children[current_level], base);
          bool found = false;
          for (preorder_t p = current_primary; p < new_current_primary; p++)
          {
            if (primary_key_list[p].check_if_present(primary_key, base)) // todo: need to restore check_if_present...
            {
              found = true;
              parent_symbol =
                  ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                      ->get_k_th_set_bit(current_node,
                                         p - current_primary /* 0-indexed*/,
                                         node_positions[current_node],
                                         level_to_num_children[current_level], base);
              break;
            }
          }
          current_primary = new_current_primary;
          if (!found && stack[sTop] > 0)
          {
            top_node = path[sTop];

            symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                               ->next_symbol(
                                   symbol[sTop] + 1,
                                   top_node,
                                   node_positions[top_node],
                                   (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                                   level_to_num_children[sTop_to_level[sTop]],
                                   base);
          }
          if (found)
          {
            break;
          }
        }
      }
      ++current_node;

      bool backtraceked = false;
      while (sTop >= 0 && stack[sTop] == 0)
      {
        backtraceked = true;
        path[sTop] = 0;
        symbol[sTop] = -1;
        --sTop;
        --current_level;
        if (sTop >= 0)
          --stack[sTop];
      }
      if (backtraceked)
      {
        top_node = path[sTop];
        symbol[sTop] = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                           ->next_symbol(
                               symbol[sTop] + 1,
                               top_node,
                               node_positions[top_node],
                               (1 << level_to_num_children[sTop_to_level[sTop]]) - 1,
                               level_to_num_children[sTop_to_level[sTop]],
                               base);
      }
    }
    if (current_node == num_nodes_)
    {
      fprintf(stderr, "node not found!\n");
      return 0;
    }
    for (int i = 0; i <= sTop; i++)
    {
      node_path[root_depth_ + i] = symbol[i];
    }
    if (root_depth_ != trie_depth_)
    {
      ((disk_tree_block<DIMENSION> *)((uint64_t)disk_parent_combined_ptr_ + base))
          ->get_node_path(treeblock_frontier_num_, node_path, base);
    }
    else
    {
      ((disk_trie_node<DIMENSION> *)((uint64_t)disk_parent_combined_ptr_ + base))
          ->get_node_path(root_depth_, node_path, base);
    }
    lookup_scanned_nodes += current_node;
    return parent_symbol;
  }

  data_point<DIMENSION> *node_path_to_coordinates(
      std::vector<morton_t> &node_path,
      dimension_t dimension) const
  {

    // Will be free-ed in the benchmark code
    auto coordinates = new data_point<DIMENSION>();

    for (level_t i = 0; i < max_depth_; i++)
    {
      morton_t current_symbol = node_path[i];
      morton_t current_symbol_pos = level_to_num_children[i] - 1;

      for (dimension_t j = 0; j < dimension; j++)
      {

        if (dimension_to_num_bits[j] <= i || i < start_dimension_bits[j])
          continue;

        level_t current_bit = GETBIT(current_symbol, current_symbol_pos);
        current_symbol_pos--;

        point_t coordinate = coordinates->get_coordinate(j);
        coordinate = (coordinate << 1) + current_bit;
        coordinates->set_coordinate(j, coordinate);
      }
    }
    if (!coordinates)
    {
      std::cerr << "TPCH: range search failed!" << std::endl;
      exit(-1);
    }
    return coordinates;
  }

  std::vector<int32_t> node_path_to_coordinates_vect(
      std::vector<morton_t> &node_path,
      dimension_t dimension) const
  {
    // Will be free-ed in the benchmark code
    std::vector<int32_t> ret_vect(dimension, 0);

    for (level_t i = 0; i < max_depth_; i++)
    {
      morton_t current_symbol = node_path[i];
      morton_t current_symbol_pos = level_to_num_children[i] - 1;

      for (dimension_t j = 0; j < dimension; j++)
      {

        if (dimension_to_num_bits[j] <= i || i < start_dimension_bits[j])
          continue;

        level_t current_bit = GETBIT(current_symbol, current_symbol_pos);
        current_symbol_pos--;

        point_t coordinate = ret_vect[j];
        coordinate = (coordinate << 1) + current_bit;
        ret_vect[j] = coordinate;
      }
    }
    return ret_vect;
  }

  void range_search_treeblock(data_point<DIMENSION> *start_range,
                              data_point<DIMENSION> *end_range,
                              disk_tree_block<DIMENSION> *current_block,
                              level_t level,
                              preorder_t current_node,
                              preorder_t current_node_pos,
                              preorder_t prev_node,
                              preorder_t prev_node_pos,
                              preorder_t current_frontier,
                              preorder_t current_primary,
                              std::vector<int32_t> &found_points,
                              uint64_t base)
  {

    if (level == max_depth_)
    {

      morton_t parent_symbol = start_range->leaf_to_symbol(max_depth_ - 1);
      morton_t tmp_symbol =
          ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
              ->next_symbol(0,
                            prev_node,
                            prev_node_pos,
                            (1 << level_to_num_children[level - 1]) - 1,
                            level_to_num_children[level - 1],
                            base);

      while (tmp_symbol != parent_symbol)
      {
        tmp_symbol =
            ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                ->next_symbol(tmp_symbol + 1,
                              prev_node,
                              prev_node_pos,
                              (1 << level_to_num_children[level - 1]) - 1,
                              level_to_num_children[level - 1], base);
        current_primary++;
      }

      // n_leaves_t list_size = primary_key_list[current_primary].size();
      n_leaves_t list_size = 1;
      for (n_leaves_t i = 0; i < list_size; i++)
      {
        for (dimension_t j = 0; j < DIMENSION; j++)
        {
          found_points.push_back(start_range->get_coordinate(j));
        }
      }
      return;
    }

    if (current_node >= num_nodes_)
    {
      return;
    }

    if (num_frontiers() > 0 && current_frontier < num_frontiers() &&
        current_node == get_preorder(current_frontier, base))
    {

      disk_tree_block<DIMENSION> *new_current_block = get_pointer(current_frontier, base);
      preorder_t new_current_frontier = 0;
      preorder_t new_current_primary = 0;
      new_current_block->range_search_treeblock(start_range,
                                                end_range,
                                                new_current_block,
                                                level,
                                                0,
                                                0,
                                                0,
                                                0,
                                                new_current_frontier,
                                                new_current_primary,
                                                found_points,
                                                base);
      return;
    }

    morton_t start_range_symbol = start_range->leaf_to_symbol(level);
    morton_t end_range_symbol = end_range->leaf_to_symbol(level);
    morton_t representation = start_range_symbol ^ end_range_symbol;
    morton_t neg_representation = ~representation;

    data_point<DIMENSION> original_start_range = (*start_range);
    data_point<DIMENSION> original_end_range = (*end_range);

    preorder_t new_current_node;
    preorder_t new_current_node_pos = 0;
    disk_tree_block<DIMENSION> *new_current_block;
    preorder_t new_current_frontier;
    preorder_t new_current_primary;

    morton_t current_symbol = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                                  ->next_symbol(start_range_symbol,
                                                current_node,
                                                current_node_pos,
                                                end_range_symbol,
                                                level_to_num_children[level], base);

    preorder_t stack_range_search[100];
    int sTop_range_search = -1;
    preorder_t current_node_pos_range_search = 0;
    preorder_t current_node_range_search = 0;
    preorder_t next_frontier_preorder_range_search = 0;
    preorder_t current_frontier_cont = 0;
    preorder_t current_primary_cont = 0;

    bare_minimum_count += end_range_symbol - current_symbol + 1;
    checked_points_count += 1;

    if (!query_optimization)
    {
      current_symbol = 0;
      end_range_symbol = end_range->leaf_to_full_symbol(level);
    }
    while (current_symbol <= end_range_symbol)
    {
      if (!((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
               ->has_symbol(current_node,
                            current_node_pos,
                            current_symbol,
                            level_to_num_children[level], base))
      {
        continue;
      }

      if (query_optimization == 1 ||
          (start_range_symbol & neg_representation) ==
              (current_symbol & neg_representation))
      {
        new_current_block = current_block;
        new_current_frontier = current_frontier;
        new_current_primary = current_primary;
        new_current_node_pos = current_node_pos;

        if (REUSE_RANGE_SEARCH_CHILD &&
            level < max_depth_ - 1 /*At least 1 levels to the bottom*/)
        {
          new_current_node = current_block->child_range_search(
              current_node,
              new_current_node_pos,
              current_symbol,
              level,
              new_current_frontier,
              new_current_primary,
              stack_range_search,
              sTop_range_search,
              current_node_pos_range_search,
              current_node_range_search,
              next_frontier_preorder_range_search,
              current_frontier_cont,
              current_primary_cont,
              base);
        }
        else
          new_current_node = current_block->child(new_current_block,
                                                  current_node,
                                                  new_current_node_pos,
                                                  current_symbol,
                                                  level,
                                                  new_current_frontier,
                                                  new_current_primary,
                                                  base);

        start_range->update_symbol(end_range, current_symbol, level);
        current_block->range_search_treeblock(start_range,
                                              end_range,
                                              current_block,
                                              level + 1,
                                              new_current_node,
                                              new_current_node_pos,
                                              current_node,
                                              current_node_pos,
                                              new_current_frontier,
                                              new_current_primary,
                                              found_points,
                                              base);

        (*start_range) = original_start_range;
        (*end_range) = original_end_range;
      }
      current_symbol = ((disk_compressed_bitmap::disk_compressed_bitmap *)((uint64_t)disk_dfuds_ + base))
                           ->next_symbol(current_symbol + 1,
                                         current_node,
                                         current_node_pos,
                                         end_range_symbol,
                                         level_to_num_children[level], base);
    }
  }

  // deleted: mutators

private:
  level_t root_depth_;
  preorder_t num_nodes_;
  preorder_t total_nodes_bits_;
  preorder_t node_capacity_;
  disk_compressed_bitmap::disk_compressed_bitmap *disk_dfuds_{};
  disk_frontier_node<DIMENSION> *frontiers_ = nullptr;
  preorder_t num_frontiers_ = 0;

  void *disk_parent_combined_ptr_ = NULL;
  preorder_t treeblock_frontier_num_ = 0;
  std::vector<disk_bits::disk_compact_ptr> primary_key_list; // todo: deal with std::vector
  // this is the missing piece, need to basically read out value, insert the offset, and then copy
};

#endif // MD_TRIE_TREE_BLOCK_H
