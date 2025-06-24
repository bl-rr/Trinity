#ifndef DISK_MD_TRIE_TRIE_NODE_H
#define DISK_MD_TRIE_TRIE_NODE_H

#include "defs.h"
#include "disk_tree_block.h"
#include <cstdlib>
#include <sys/mman.h>

template <dimension_t DIMENSION>
class disk_trie_node
{

public:
  // deleted: constructors
  // deleted: mutators

  inline disk_tree_block<DIMENSION> *get_block(uint64_t base) const
  {
    return (disk_tree_block<DIMENSION> *)((uint64_t)disk_trie_or_treeblock_ptr_ + base);
  }

  inline disk_trie_node<DIMENSION> *get_child(morton_t symbol, uint64_t base)
  {
    auto trie_ptr = (trie_node<DIMENSION> **)((uint64_t)disk_trie_or_treeblock_ptr_ + base);
    return (disk_trie_node<DIMENSION> *)((uint64_t)trie_ptr[symbol] + base);
  }

  void get_node_path(level_t level, std::vector<morton_t> &node_path, uint64_t base)
  {

    if ((trie_node<DIMENSION> *)((uint64_t)disk_parent_trie_node_ + base))
    {
      node_path[level - 1] = parent_symbol_;
      ((trie_node<DIMENSION> *)((uint64_t)disk_parent_trie_node_ + base))->get_node_path(level - 1, node_path);
    }
  }

  disk_trie_node<DIMENSION> *get_parent_trie_node(uint64_t base) { return (trie_node<DIMENSION> *)((uint64_t)disk_parent_trie_node_ + base); }

  morton_t get_parent_symbol() { return parent_symbol_; }

  // deleted: size

private:
  void *disk_trie_or_treeblock_ptr_ = NULL;
  disk_trie_node<DIMENSION> *disk_parent_trie_node_ = NULL;
  morton_t parent_symbol_ = 0;
};

#endif // MD_TRIE_TRIE_NODE_H
