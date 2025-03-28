#ifndef MD_TRIE_TRIE_NODE_H
#define MD_TRIE_TRIE_NODE_H

#include "defs.h"
#include "tree_block.h"
#include <cstdlib>
#include <sys/mman.h>

template <dimension_t DIMENSION> class trie_node {

  public:
    explicit trie_node(bool is_leaf, dimension_t num_dimensions) {
        if (!is_leaf) {
            trie_or_treeblock_ptr_ = (trie_node<DIMENSION> **)calloc(
                1 << num_dimensions, sizeof(trie_node<DIMENSION> *));
        }
    }

    inline trie_node<DIMENSION> *get_child(morton_t symbol) {
        auto trie_ptr = (trie_node<DIMENSION> **)trie_or_treeblock_ptr_;
        return trie_ptr[symbol];
    }

    inline void set_child(morton_t symbol, trie_node *node) {
        auto trie_ptr = (trie_node<DIMENSION> **)trie_or_treeblock_ptr_;
        trie_ptr[symbol] = node;
    }

    inline tree_block<DIMENSION> *get_block() const {
        return (tree_block<DIMENSION> *)trie_or_treeblock_ptr_;
    }

    inline void set_block(tree_block<DIMENSION> *block) {
        trie_or_treeblock_ptr_ = block;
    }

    trie_node<DIMENSION> *get_parent_trie_node() { return parent_trie_node_; }

    void set_parent_trie_node(trie_node<DIMENSION> *node) {

        parent_trie_node_ = node;
    }

    morton_t get_parent_symbol() { return parent_symbol_; }

    void set_parent_symbol(morton_t symbol) { parent_symbol_ = symbol; }

    uint64_t size(level_t num_children, bool is_leaf) {

        uint64_t total_size = sizeof(trie_or_treeblock_ptr_);
        total_size += sizeof(parent_trie_node_); // parent_trie_node_
        total_size += sizeof(parent_symbol_);    // parent_symbol_

        if (is_leaf)
            total_size += sizeof(trie_node<DIMENSION> *) * num_children;

        return total_size;
    }

    /// @brief
    /// @param file
    /// @param level
    /// @param node_offset the offset to file at which the node should be
    ///                    inserted
    /// @param temp_node a previously wiped out trie_node<DIMENSION> that could
    ///                  be reused, it is this function's responsibility to free
    void serialize(FILE *file, level_t level, uint64_t node_offset,
                   trie_node<DIMENSION> *temp_node) {

        // ensure we have allocated space for this node
        assert(current_offset > node_offset);

        // each node should only be serialized once
        assert(pointers_to_offsets_map.find((uint64_t)this) ==
               pointers_to_offsets_map.end());

        // the location of insertion is known at this point, store (this ->
        // offset) in the map and update the offset
        pointers_to_offsets_map.insert({(uint64_t)this, node_offset});

        // again, reuse the buffer for easy modification of the node copy
        memcpy(temp_node, this, sizeof(trie_node<DIMENSION>));

        // populate parent_trie_node_ if the node is not root
        if (!this->parent_trie_node_)
            assert(level == 0);
        else
            // else the parent_trie_node_ should be in the map
            temp_node->parent_trie_node_ =
                (trie_node<DIMENSION> *)pointers_to_offsets_map.at(
                    (uint64_t)(this->get_parent_trie_node()));

        // populate temp_node for insertion, but trie_node's nature differs with
        // level:
        //      1) leaf -> tree_block
        //      2) internal -> list of trie_node<DIMENSION> *
        if (level == trie_depth_) {
            // at this point, the trie_node is probably at the end of the
            // insertion queue, meaning it can be inserted right away
            assert((current_offset - sizeof(trie_node<DIMENSION>)) ==
                   node_offset);

            // this is a leaf node and should have a tree_block that needs
            // serializing
            assert(this->get_block());
            // this block should not have been allocated yet
            assert(
                pointers_to_offsets_map.find((uint64_t)(this->get_block())) ==
                pointers_to_offsets_map.end());

            // we need to create the block on file, and
            // `current_offset` is the next writing position
            temp_node->trie_or_treeblock_ptr_ = (void *)current_offset;

            // node has all the necessary info, write the node at the
            // position `node_offset`
            if (ftell(file) != node_offset) {
                fseek(file, node_offset, SEEK_SET);
                fwrite(temp_node, sizeof(trie_node<DIMENSION>), 1, file);
                fseek(file, 0, SEEK_END);
            } else
                fwrite(temp_node, sizeof(trie_node<DIMENSION>), 1, file);

            free(temp_node);

            // write empty space for treeblock
            tree_block<DIMENSION> *wipe_out_treeblock =
                (tree_block<DIMENSION> *)calloc(1,
                                                sizeof(tree_block<DIMENSION>));
            fwrite(wipe_out_treeblock, sizeof(tree_block<DIMENSION>), 1, file);
            uint64_t treeblock_offset = current_offset;
            current_offset += sizeof(tree_block<DIMENSION>);

            this->get_block()->serialize(file, treeblock_offset,
                                         wipe_out_treeblock);
        } else {
            // this is not a leafnode, but a list of
            //      trie_node<DIMENSION> *
            // of size
            //      1 << level_to_num_children[level]

            // create the big chunk
            trie_node<DIMENSION> **temp_children =
                (trie_node<DIMENSION> **)calloc(
                    1 << level_to_num_children[level],
                    sizeof(trie_node<DIMENSION> *));
            // write to file
            fwrite(temp_children, sizeof(trie_node<DIMENSION> *),
                   1 << level_to_num_children[level], file);
            uint64_t temp_children_offset = current_offset;
            pointers_to_offsets_map.insert(
                {(uint64_t)this->trie_or_treeblock_ptr_, temp_children_offset});
            current_offset += sizeof(trie_node<DIMENSION> *) *
                              (1 << level_to_num_children[level]);

            // don't need to copy here, because it's all pointers, and other
            // wise intialised to be nullptr anyway
            // memcpy(temp_children,
            //        (trie_node<DIMENSION> **)this->trie_or_treeblock_ptr_,
            //        sizeof(trie_node<DIMENSION> *) *
            //            (1 << level_to_num_children[level]));

            // create dummy node to write empty space
            trie_node<DIMENSION> *wipe_out_trie_node =
                (trie_node<DIMENSION> *)calloc(1, sizeof(trie_node<DIMENSION>));

            // create space for all empty nodes
            for (int i = 0; i < (1 << level_to_num_children[level]); i++) {
                if (this->get_child(i)) {
                    // the child node should not have been created yet
                    assert(pointers_to_offsets_map.find(
                               (uint64_t)(this->get_child(i))) ==
                           pointers_to_offsets_map.end());

                    temp_children[i] = (trie_node<DIMENSION> *)current_offset;
                    uint64_t child_offset = current_offset;

                    fwrite(wipe_out_trie_node, sizeof(trie_node<DIMENSION>), 1,
                           file);
                    current_offset += sizeof(trie_node<DIMENSION>);

                    this->get_child(i)->serialize(
                        file, level + 1, child_offset,
                        (trie_node<DIMENSION> *)calloc(
                            1, sizeof(trie_node<DIMENSION>)));
                }
            }

            free(wipe_out_trie_node);

            // write the big chunk of list of trie_node<DIMENSION> *'s at
            // `temp_children_offset`
            if (ftell(file) != temp_children_offset) {
                fseek(file, temp_children_offset, SEEK_SET);
                fwrite(temp_children, sizeof(trie_node<DIMENSION> *),
                       1 << level_to_num_children[level], file);
                fseek(file, 0, SEEK_END);
            } else
                fwrite(temp_children, sizeof(trie_node<DIMENSION> *),
                       1 << level_to_num_children[level], file);

            free(temp_children);

            // populate the trie_node with the list of trie_node<DIMENSION> *
            temp_node->trie_or_treeblock_ptr_ = (void *)temp_children_offset;

            // write the node at the position node_offset
            if (ftell(file) != node_offset) {
                fseek(file, node_offset, SEEK_SET);
                fwrite(temp_node, sizeof(trie_node<DIMENSION>), 1, file);
                fseek(file, 0, SEEK_END);
            } else
                fwrite(temp_node, sizeof(trie_node<DIMENSION>), 1, file);

            free(temp_node);
        }
    }

    void deserialize(level_t level, uint64_t base_addr) {
        if (parent_trie_node_) {
            assert(level);
            parent_trie_node_ =
                (trie_node<DIMENSION> *)(base_addr +
                                         (uint64_t)parent_trie_node_);
        }

        if (trie_or_treeblock_ptr_) {
            // update the pointer to the correct location
            trie_or_treeblock_ptr_ =
                (void *)(base_addr + (uint64_t)trie_or_treeblock_ptr_);

            if (level == trie_depth_) {
                // if leaf, then should be a block
                get_block()->deserialize(base_addr);
            } else {
                // if internal, then should be a list of trie_node<DIMENSION> *
                trie_node<DIMENSION> **children =
                    (trie_node<DIMENSION> **)trie_or_treeblock_ptr_;
                for (int i = 0; i < (1 << level_to_num_children[level]); i++) {
                    if (children[i]) {
                        children[i] =
                            (trie_node<DIMENSION> *)(base_addr +
                                                     (uint64_t)children[i]);
                        children[i]->deserialize(level + 1, base_addr);
                    }
                }
            }
        }
    }

  private:
    void *trie_or_treeblock_ptr_ = nullptr;
    trie_node<DIMENSION> *parent_trie_node_ = nullptr;
    morton_t parent_symbol_ = 0;

    // Grant md_trie access to private members.
    template <dimension_t> friend class md_trie;
};

#endif // MD_TRIE_TRIE_NODE_H
