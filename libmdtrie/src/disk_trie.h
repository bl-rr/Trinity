#ifndef DISK_MD_TRIE_MD_TRIE_H
#define DISK_MD_TRIE_MD_TRIE_H

#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <queue>
#include <sys/stat.h>
#include <utility>
#include <vector>

#include "data_point.h"
#include "defs.h"
#include "disk_tree_block.h"
#include "disk_trie_node.h"
#include "disk_compressed_bitmap.h"

template <dimension_t DIMENSION>
class disk_md_trie
{
public:
  // deleted: constructors

  inline disk_trie_node<DIMENSION> *root(uint64_t base) { return (disk_trie_node<DIMENSION> *)(base + (uint64_t)disk_root_); }

  // current_trie_node should be already not on the disk
  disk_tree_block<DIMENSION> *walk_trie(disk_trie_node<DIMENSION> *current_trie_node,
                                        data_point<DIMENSION> *leaf_point,
                                        level_t &level,
                                        uint64_t base) const
  {

    morton_t current_symbol;

    while (level < trie_depth_ &&
           current_trie_node->get_child(leaf_point->leaf_to_symbol(level), base))
    {

      current_trie_node =
          current_trie_node->get_child(leaf_point->leaf_to_symbol(level), base);
      level++;
    }
    while (level < trie_depth_)
    {
      // should not insert, point not found
      return nullptr;
    }

    disk_tree_block<DIMENSION> *current_treeblock = nullptr;
    if (current_trie_node->get_block() == nullptr)
    {
      return nullptr;
    }
    else
    {
      current_treeblock =
          (disk_tree_block<DIMENSION> *)current_trie_node->get_block(base);
    }
    return current_treeblock;
  }

  data_point<DIMENSION> *disk_lookup_trie(n_leaves_t primary_key, disk_bitmap::disk_CompactPtrVector *p_key_to_treeblock_compact, uint64_t base)
  {
    std::vector<morton_t> node_path_from_primary(max_depth_ + 1);
    disk_tree_block<DIMENSION> *t_ptr = (disk_tree_block<DIMENSION> *)p_key_to_treeblock_compact->At(primary_key, base);
    morton_t parent_symbol_from_primary =
        t_ptr->get_node_path_primary_key(primary_key, node_path_from_primary, base);
    node_path_from_primary[max_depth_ - 1] = parent_symbol_from_primary;
    return t_ptr->node_path_to_coordinates(node_path_from_primary, 9);
  }

  void disk_range_search_trie(data_point<DIMENSION> *start_range,
                              data_point<DIMENSION> *end_range,
                              disk_trie_node<DIMENSION> *current_trie_node,
                              level_t level,
                              std::vector<int32_t> &found_points,
                              uint64_t base)
  {
    if (level == trie_depth_)
    {

      disk_tree_block<DIMENSION> *current_treeblock =
          (disk_tree_block<DIMENSION> *)current_trie_node->get_block(base);
      current_treeblock->range_search_treeblock(start_range,
                                                end_range,
                                                current_treeblock,
                                                level,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                found_points,
                                                base);
      return;
    }

    morton_t start_symbol = start_range->leaf_to_symbol(level);
    morton_t end_symbol = end_range->leaf_to_symbol(level);
    morton_t representation = start_symbol ^ end_symbol;
    morton_t neg_representation = ~representation;

    struct data_point<DIMENSION> original_start_range = (*start_range);
    struct data_point<DIMENSION> original_end_range = (*end_range);
    for (morton_t current_symbol = start_symbol; current_symbol <= end_symbol;
         current_symbol++)
    {

      if ((start_symbol & neg_representation) !=
          (current_symbol & neg_representation))
      {
        continue;
      }

      if (!current_trie_node->get_child(current_symbol, base))
      {
        continue;
      }

      start_range->update_symbol(end_range, current_symbol, level);

      disk_range_search_trie(start_range,
                             end_range,
                             current_trie_node->get_child(current_symbol, base),
                             level + 1,
                             found_points, base);
      (*start_range) = original_start_range;
      (*end_range) = original_end_range;
    }
  }

private:
  disk_trie_node<DIMENSION> *disk_root_ = nullptr;
  level_t max_depth_;
  preorder_t max_tree_nodes_;
};

#endif // MD_TRIE_MD_TRIE_H
