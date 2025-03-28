#ifndef MD_TRIE_MD_TRIE_H
#define MD_TRIE_MD_TRIE_H

#include "compressed_bitmap.h"
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
#include "tree_block.h"
#include "trie_node.h"

template <dimension_t DIMENSION> class md_trie {
  public:
    explicit md_trie(level_t max_depth, level_t trie_depth,
                     preorder_t max_tree_nodes) {

        max_depth_ = max_depth;
        trie_depth_ = trie_depth;
        max_tree_nodes_ = max_tree_nodes;
        root_ = new trie_node<DIMENSION>(false, level_to_num_children[0]);
    }

    // The md_trie destructor calls our recursive cleanup helper.
    ~md_trie() { free_node(root_, 0); }

    inline trie_node<DIMENSION> *root() { return root_; }

    tree_block<DIMENSION> *walk_trie(trie_node<DIMENSION> *current_trie_node,
                                     data_point<DIMENSION> *leaf_point,
                                     level_t &level) const {

        morton_t current_symbol;

        while (level < trie_depth_ && current_trie_node->get_child(
                                          leaf_point->leaf_to_symbol(level))) {

            current_trie_node =
                current_trie_node->get_child(leaf_point->leaf_to_symbol(level));
            level++;
        }
        while (level < trie_depth_) {

            current_symbol = leaf_point->leaf_to_symbol(level);
            if (level == trie_depth_ - 1) {
                current_trie_node->set_child(
                    current_symbol,
                    new trie_node<DIMENSION>(true,
                                             level_to_num_children[level + 1]));
            } else {
                current_trie_node->set_child(
                    current_symbol,
                    new trie_node<DIMENSION>(false,
                                             level_to_num_children[level + 1]));
            }
            current_trie_node->get_child(current_symbol)
                ->set_parent_trie_node(current_trie_node);
            current_trie_node->get_child(current_symbol)
                ->set_parent_symbol(current_symbol);
            current_trie_node = current_trie_node->get_child(current_symbol);
            level++;
        }

        tree_block<DIMENSION> *current_treeblock = nullptr;
        if (current_trie_node->get_block() == nullptr) {
            current_treeblock = new tree_block<DIMENSION>(
                trie_depth_, 1 /* initial_tree_capacity_ */,
                1 << level_to_num_children[trie_depth_], 1, max_depth_,
                max_tree_nodes_, current_trie_node);
            current_trie_node->set_block(current_treeblock);
        } else {
            current_treeblock =
                (tree_block<DIMENSION> *)current_trie_node->get_block();
        }
        return current_treeblock;
    }

    void insert_trie(data_point<DIMENSION> *leaf_point,
                     n_leaves_t primary_key) {

        level_t level = 0;
        trie_node<DIMENSION> *current_trie_node = root_;
        tree_block<DIMENSION> *current_treeblock =
            walk_trie(current_trie_node, leaf_point, level);
        current_treeblock->insert_remaining(leaf_point, level, primary_key);
    }

    bool check(data_point<DIMENSION> *leaf_point) const {

        level_t level = 0;
        trie_node<DIMENSION> *current_trie_node = root_;
        tree_block<DIMENSION> *current_treeblock =
            walk_trie(current_trie_node, leaf_point, level);
        bool result = current_treeblock->walk_tree_block(leaf_point, level);
        return result;
    }

    uint64_t size() {

        uint64_t total_size = sizeof(root_) + sizeof(max_depth_);
        total_size += sizeof(max_tree_nodes_);

        std::queue<trie_node<DIMENSION> *> trie_node_queue;
        trie_node_queue.push(root_);
        level_t current_level = 0;

        while (!trie_node_queue.empty()) {

            unsigned int queue_size = trie_node_queue.size();

            for (unsigned int s = 0; s < queue_size; s++) {

                trie_node<DIMENSION> *current_node = trie_node_queue.front();
                trie_node_queue.pop();

                total_size += current_node->size(
                    1 << level_to_num_children[current_level],
                    current_level == trie_depth_);

                if (current_level != trie_depth_) {
                    for (int i = 0;
                         i < (1 << level_to_num_children[current_level]); i++) {
                        if (current_node->get_child(i)) {
                            trie_node_queue.push(current_node->get_child(i));
                        }
                    }
                } else {
                    total_size += current_node->get_block()->size();
                }
            }
            current_level++;
        }
        // Global variables in def.h
        total_size += sizeof(total_points_count);
        total_size += sizeof(discount_factor);
        total_size +=
            sizeof(level_to_num_children) + max_depth_ * sizeof(morton_t);
        total_size += sizeof(max_tree_nodes_);
        total_size += sizeof(max_depth_);
        total_size += sizeof(trie_depth_);

        total_size += sizeof(dimension_to_num_bits) +
                      dimension_to_num_bits.size() * sizeof(morton_t);
        total_size += sizeof(start_dimension_bits) +
                      start_dimension_bits.size() * sizeof(level_t);
        total_size += sizeof(no_dynamic_sizing);

        return total_size;
    }

    void range_search_trie(data_point<DIMENSION> *start_range,
                           data_point<DIMENSION> *end_range,
                           trie_node<DIMENSION> *current_trie_node,
                           level_t level,
                           device_vector<int32_t> &found_points) {
        if (level == trie_depth_) {

            auto *current_treeblock =
                (tree_block<DIMENSION> *)current_trie_node->get_block();
            current_treeblock->range_search_treeblock(
                start_range, end_range, current_treeblock, level, 0, 0, 0, 0, 0,
                0, found_points);
            return;
        }

        morton_t start_symbol = start_range->leaf_to_symbol(level);
        morton_t end_symbol = end_range->leaf_to_symbol(level);
        morton_t representation = start_symbol ^ end_symbol;
        morton_t neg_representation = ~representation;

        struct data_point<DIMENSION> original_start_range = (*start_range);
        struct data_point<DIMENSION> original_end_range = (*end_range);
        for (morton_t current_symbol = start_symbol;
             current_symbol <= end_symbol; current_symbol++) {

            if ((start_symbol & neg_representation) !=
                (current_symbol & neg_representation)) {
                continue;
            }

            if (!current_trie_node->get_child(current_symbol)) {
                continue;
            }

            start_range->update_symbol(end_range, current_symbol, level);

            range_search_trie(start_range, end_range,
                              current_trie_node->get_child(current_symbol),
                              level + 1, found_points);
            (*start_range) = original_start_range;
            (*end_range) = original_end_range;
        }
    }

    /// @brief starts to serialize the trie at the `current_offset`,
    ///        it is caller's responsibility to align the file cursor
    /// @param file
    void serialize(FILE *file) {

        // asserting that the trie is not already inserted, each data structure
        // should only be inserted once
        assert(pointers_to_offsets_map.find((uint64_t)this) ==
               pointers_to_offsets_map.end());

        // current_offset should be the location where the trie is written
        pointers_to_offsets_map.insert({(uint64_t)this, current_offset});

        // create a buffer for easy modification of the trie copy
        md_trie *temp_trie = (md_trie *)malloc(sizeof(md_trie<DIMENSION>));
        memcpy(temp_trie, this, sizeof(md_trie<DIMENSION>));

        // create root node offset, after where the trie would live
        uint64_t root_offset = current_offset + sizeof(md_trie<DIMENSION>);
        if (this->root_)
            temp_trie->root_ = (trie_node<DIMENSION> *)(root_offset);
        else {
            fwrite(temp_trie, sizeof(md_trie<DIMENSION>), 1, file);
            free(temp_trie);
            return;
        }

        // perform write for normal case
        // current_offset should always be the next write offset
        fwrite(temp_trie, sizeof(md_trie<DIMENSION>), 1, file);
        free(temp_trie);

        current_offset += sizeof(md_trie<DIMENSION>);

        // create a temp buffer and write empty bytes in place of the root node
        // to be written later
        trie_node<DIMENSION> *temp_root =
            (trie_node<DIMENSION> *)calloc(1, sizeof(trie_node<DIMENSION>));
        fwrite(temp_root, sizeof(trie_node<DIMENSION>), 1, file);

        // now match `current_offset` with FILE CURSOR
        current_offset += sizeof(trie_node<DIMENSION>);

        // this is fine, the next node is guaranteed to not have been created
        root_->serialize(file, 0, root_offset, temp_root);
    }

    void deserialize(uint64_t base_addr) {
        if (root_) {
            root_ = (trie_node<DIMENSION> *)(base_addr + (uint64_t)root_);
            root_->deserialize(0, base_addr);
        }
    }

  private:
    // NOTE: we removed p_key_to_treeblock_compact, meaning that we can no
    // longer support duplicate primary keys

    trie_node<DIMENSION> *root_ = nullptr;
    level_t max_depth_;
    preorder_t max_tree_nodes_;

    // Helper function to recursively free a node.
    void free_node(trie_node<DIMENSION> *node, level_t level) {
        if (node == nullptr)
            return;

        if (level < trie_depth_) {
            // Internal node: the pointer stores an array of child pointers.
            trie_node<DIMENSION> **children =
                reinterpret_cast<trie_node<DIMENSION> **>(
                    node->trie_or_treeblock_ptr_);
            dimension_t num_children =
                1 << level_to_num_children[level]; // Known from your allocation
            for (dimension_t i = 0; i < num_children; i++) {
                free_node(children[i], level + 1);
            }
            // Free the children array allocated with calloc.
            free(node->trie_or_treeblock_ptr_);
        } else {
            // Leaf node: the pointer stores a tree_block allocated via new.
            tree_block<DIMENSION> *block =
                reinterpret_cast<tree_block<DIMENSION> *>(
                    node->trie_or_treeblock_ptr_);
            delete block;
        }
        // Finally, delete the node itself.
        delete node;
    }

    // void serialize_node(trie_node<DIMENSION> *current_trie_node, level_t
    // level,
    //                     FILE *file) {
    //     if (current_trie_node == nullptr) {
    //         return;
    //     }

    //     // always write the node itself first
    //     current_trie_node->serialize(file, level);

    //     // now we decide if we are at the end of the trie
    //     if (level < trie_depth_) {
    //         dimension_t num_children = 1 << level_to_num_children[level];

    //         // for each of the valid child, we serialize the child node,
    //         though
    //         // it may have already been created
    //         for (dimension_t i = 0; i < num_children; i++) {
    //             if (current_trie_node->get_child(i)) {
    //                 serialize_node(current_trie_node->get_child(i), level +
    //                 1,
    //                                file);
    //             }
    //         }
    //     }

    //     // else, we are at the end, and just return
    // }
};

#endif // MD_TRIE_MD_TRIE_H
