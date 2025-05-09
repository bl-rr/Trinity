#ifndef BITMAP_COMPRESSED_BITMAP_H_
#define BITMAP_COMPRESSED_BITMAP_H_

#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <defs.h>
#include <iostream>
#include <signal.h>
#include <x86intrin.h>

namespace compressed_bitmap {

#define GETBIT(n, i) ((n >> i) & 1UL)
#define SETBIT(n, i) n = (n | (1UL << i))
#define CLRBIT(n, i) n = (n & ~(1UL << i))

#define BITS2BLOCKS(bits)                                                      \
    (((bits) % 64 == 0) ? ((bits) / 64) : (((bits) / 64) + 1))

#define GETBITVAL(data, i) GETBIT((data)[(i) / 64], (i) % 64)
#define SETBITVAL(data, i) SETBIT((data)[(i) / 64], (i) % 64)
#define CLRBITVAL(data, i) CLRBIT((data)[(i) / 64], (i) % 64)

const uint64_t all_set = -1ULL;

static const uint64_t high_bits_set[65] = {
    0x0000000000000000ULL, 0x8000000000000000ULL, 0xC000000000000000ULL,
    0xE000000000000000ULL, 0xF000000000000000ULL, 0xF800000000000000ULL,
    0xFC00000000000000ULL, 0xFE00000000000000ULL, 0xFF00000000000000ULL,
    0xFF80000000000000ULL, 0xFFC0000000000000ULL, 0xFFE0000000000000ULL,
    0xFFF0000000000000ULL, 0xFFF8000000000000ULL, 0xFFFC000000000000ULL,
    0xFFFE000000000000ULL, 0xFFFF000000000000ULL, 0xFFFF800000000000ULL,
    0xFFFFC00000000000ULL, 0xFFFFE00000000000ULL, 0xFFFFF00000000000ULL,
    0xFFFFF80000000000ULL, 0xFFFFFC0000000000ULL, 0xFFFFFE0000000000ULL,
    0xFFFFFF0000000000ULL, 0xFFFFFF8000000000ULL, 0xFFFFFFC000000000ULL,
    0xFFFFFFE000000000ULL, 0xFFFFFFF000000000ULL, 0xFFFFFFF800000000ULL,
    0xFFFFFFFC00000000ULL, 0xFFFFFFFE00000000ULL, 0xFFFFFFFF00000000ULL,
    0xFFFFFFFF80000000ULL, 0xFFFFFFFFC0000000ULL, 0xFFFFFFFFE0000000ULL,
    0xFFFFFFFFF0000000ULL, 0xFFFFFFFFF8000000ULL, 0xFFFFFFFFFC000000ULL,
    0xFFFFFFFFFE000000ULL, 0xFFFFFFFFFF000000ULL, 0xFFFFFFFFFF800000ULL,
    0xFFFFFFFFFFC00000ULL, 0xFFFFFFFFFFE00000ULL, 0xFFFFFFFFFFF00000ULL,
    0xFFFFFFFFFFF80000ULL, 0xFFFFFFFFFFFC0000ULL, 0xFFFFFFFFFFFE0000ULL,
    0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFFFF8000ULL, 0xFFFFFFFFFFFFC000ULL,
    0xFFFFFFFFFFFFE000ULL, 0xFFFFFFFFFFFFF000ULL, 0xFFFFFFFFFFFFF800ULL,
    0xFFFFFFFFFFFFFC00ULL, 0xFFFFFFFFFFFFFE00ULL, 0xFFFFFFFFFFFFFF00ULL,
    0xFFFFFFFFFFFFFF80ULL, 0xFFFFFFFFFFFFFFC0ULL, 0xFFFFFFFFFFFFFFE0ULL,
    0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFF8ULL, 0xFFFFFFFFFFFFFFFCULL,
    0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL};

static const uint64_t high_bits_unset[65] = {
    0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL, 0x3FFFFFFFFFFFFFFFULL,
    0x1FFFFFFFFFFFFFFFULL, 0x0FFFFFFFFFFFFFFFULL, 0x07FFFFFFFFFFFFFFULL,
    0x03FFFFFFFFFFFFFFULL, 0x01FFFFFFFFFFFFFFULL, 0x00FFFFFFFFFFFFFFULL,
    0x007FFFFFFFFFFFFFULL, 0x003FFFFFFFFFFFFFULL, 0x001FFFFFFFFFFFFFULL,
    0x000FFFFFFFFFFFFFULL, 0x0007FFFFFFFFFFFFULL, 0x0003FFFFFFFFFFFFULL,
    0x0001FFFFFFFFFFFFULL, 0x0000FFFFFFFFFFFFULL, 0x00007FFFFFFFFFFFULL,
    0x00003FFFFFFFFFFFULL, 0x00001FFFFFFFFFFFULL, 0x00000FFFFFFFFFFFULL,
    0x000007FFFFFFFFFFULL, 0x000003FFFFFFFFFFULL, 0x000001FFFFFFFFFFULL,
    0x000000FFFFFFFFFFULL, 0x0000007FFFFFFFFFULL, 0x0000003FFFFFFFFFULL,
    0x0000001FFFFFFFFFULL, 0x0000000FFFFFFFFFULL, 0x00000007FFFFFFFFULL,
    0x00000003FFFFFFFFULL, 0x00000001FFFFFFFFULL, 0x00000000FFFFFFFFULL,
    0x000000007FFFFFFFULL, 0x000000003FFFFFFFULL, 0x000000001FFFFFFFULL,
    0x000000000FFFFFFFULL, 0x0000000007FFFFFFULL, 0x0000000003FFFFFFULL,
    0x0000000001FFFFFFULL, 0x0000000000FFFFFFULL, 0x00000000007FFFFFULL,
    0x00000000003FFFFFULL, 0x00000000001FFFFFULL, 0x00000000000FFFFFULL,
    0x000000000007FFFFULL, 0x000000000003FFFFULL, 0x000000000001FFFFULL,
    0x000000000000FFFFULL, 0x0000000000007FFFULL, 0x0000000000003FFFULL,
    0x0000000000001FFFULL, 0x0000000000000FFFULL, 0x00000000000007FFULL,
    0x00000000000003FFULL, 0x00000000000001FFULL, 0x00000000000000FFULL,
    0x000000000000007FULL, 0x000000000000003FULL, 0x000000000000001FULL,
    0x000000000000000FULL, 0x0000000000000007ULL, 0x0000000000000003ULL,
    0x0000000000000001ULL, 0x0000000000000000ULL};

static const uint64_t low_bits_set[65] = {
    0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL,
    0x0000000000000007ULL, 0x000000000000000FULL, 0x000000000000001FULL,
    0x000000000000003FULL, 0x000000000000007FULL, 0x00000000000000FFULL,
    0x00000000000001FFULL, 0x00000000000003FFULL, 0x00000000000007FFULL,
    0x0000000000000FFFULL, 0x0000000000001FFFULL, 0x0000000000003FFFULL,
    0x0000000000007FFFULL, 0x000000000000FFFFULL, 0x000000000001FFFFULL,
    0x000000000003FFFFULL, 0x000000000007FFFFULL, 0x00000000000FFFFFULL,
    0x00000000001FFFFFULL, 0x00000000003FFFFFULL, 0x00000000007FFFFFULL,
    0x0000000000FFFFFFULL, 0x0000000001FFFFFFULL, 0x0000000003FFFFFFULL,
    0x0000000007FFFFFFULL, 0x000000000FFFFFFFULL, 0x000000001FFFFFFFULL,
    0x000000003FFFFFFFULL, 0x000000007FFFFFFFULL, 0x00000000FFFFFFFFULL,
    0x00000001FFFFFFFFULL, 0x00000003FFFFFFFFULL, 0x00000007FFFFFFFFULL,
    0x0000000FFFFFFFFFULL, 0x0000001FFFFFFFFFULL, 0x0000003FFFFFFFFFULL,
    0x0000007FFFFFFFFFULL, 0x000000FFFFFFFFFFULL, 0x000001FFFFFFFFFFULL,
    0x000003FFFFFFFFFFULL, 0x000007FFFFFFFFFFULL, 0x00000FFFFFFFFFFFULL,
    0x00001FFFFFFFFFFFULL, 0x00003FFFFFFFFFFFULL, 0x00007FFFFFFFFFFFULL,
    0x0000FFFFFFFFFFFFULL, 0x0001FFFFFFFFFFFFULL, 0x0003FFFFFFFFFFFFULL,
    0x0007FFFFFFFFFFFFULL, 0x000FFFFFFFFFFFFFULL, 0x001FFFFFFFFFFFFFULL,
    0x003FFFFFFFFFFFFFULL, 0x007FFFFFFFFFFFFFULL, 0x00FFFFFFFFFFFFFFULL,
    0x01FFFFFFFFFFFFFFULL, 0x03FFFFFFFFFFFFFFULL, 0x07FFFFFFFFFFFFFFULL,
    0x0FFFFFFFFFFFFFFFULL, 0x1FFFFFFFFFFFFFFFULL, 0x3FFFFFFFFFFFFFFFULL,
    0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};

static const uint64_t low_bits_unset[65] = {
    0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFCULL,
    0xFFFFFFFFFFFFFFF8ULL, 0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFE0ULL,
    0xFFFFFFFFFFFFFFC0ULL, 0xFFFFFFFFFFFFFF80ULL, 0xFFFFFFFFFFFFFF00ULL,
    0xFFFFFFFFFFFFFE00ULL, 0xFFFFFFFFFFFFFC00ULL, 0xFFFFFFFFFFFFF800ULL,
    0xFFFFFFFFFFFFF000ULL, 0xFFFFFFFFFFFFE000ULL, 0xFFFFFFFFFFFFC000ULL,
    0xFFFFFFFFFFFF8000ULL, 0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFFFE0000ULL,
    0xFFFFFFFFFFFC0000ULL, 0xFFFFFFFFFFF80000ULL, 0xFFFFFFFFFFF00000ULL,
    0xFFFFFFFFFFE00000ULL, 0xFFFFFFFFFFC00000ULL, 0xFFFFFFFFFF800000ULL,
    0xFFFFFFFFFF000000ULL, 0xFFFFFFFFFE000000ULL, 0xFFFFFFFFFC000000ULL,
    0xFFFFFFFFF8000000ULL, 0xFFFFFFFFF0000000ULL, 0xFFFFFFFFE0000000ULL,
    0xFFFFFFFFC0000000ULL, 0xFFFFFFFF80000000ULL, 0xFFFFFFFF00000000ULL,
    0xFFFFFFFE00000000ULL, 0xFFFFFFFC00000000ULL, 0xFFFFFFF800000000ULL,
    0xFFFFFFF000000000ULL, 0xFFFFFFE000000000ULL, 0xFFFFFFC000000000ULL,
    0xFFFFFF8000000000ULL, 0xFFFFFF0000000000ULL, 0xFFFFFE0000000000ULL,
    0xFFFFFC0000000000ULL, 0xFFFFF80000000000ULL, 0xFFFFF00000000000ULL,
    0xFFFFE00000000000ULL, 0xFFFFC00000000000ULL, 0xFFFF800000000000ULL,
    0xFFFF000000000000ULL, 0xFFFE000000000000ULL, 0xFFFC000000000000ULL,
    0xFFF8000000000000ULL, 0xFFF0000000000000ULL, 0xFFE0000000000000ULL,
    0xFFC0000000000000ULL, 0xFF80000000000000ULL, 0xFF00000000000000ULL,
    0xFE00000000000000ULL, 0xFC00000000000000ULL, 0xF800000000000000ULL,
    0xF000000000000000ULL, 0xE000000000000000ULL, 0xC000000000000000ULL,
    0x8000000000000000ULL, 0x0000000000000000ULL};

class compressed_bitmap {
  public:
    typedef uint64_t pos_type;
    typedef uint64_t size_type;
    typedef uint64_t data_type;
    typedef uint64_t width_type;

    explicit compressed_bitmap(width_type flag_size, width_type data_size) {

        data_ = (data_type *)calloc(BITS2BLOCKS(data_size), sizeof(data_type));
        flag_ = (data_type *)calloc(BITS2BLOCKS(flag_size), sizeof(data_type));
        data_size_ = data_size;
        flag_size_ = flag_size;
        SETBITVAL(flag_, 0);
    }

    ~compressed_bitmap() {
        if (data_size_ > 0) {
            free(data_);
            data_ = nullptr;
        }

        if (flag_size_ > 0) {
            free(flag_);
            flag_ = nullptr;
        }
    }

    inline uint64_t size() const {

        uint64_t total_size = 0;
        total_size += sizeof(data_size_) + sizeof(flag_size_);
        total_size += sizeof(data_) + sizeof(flag_);
        total_size += sizeof(data_type) *
                      (BITS2BLOCKS(data_size_) + BITS2BLOCKS(flag_size_));
        return total_size;
    }

    inline void increase_bits(width_type increase_width, bool is_on_data) {

        if (is_on_data) {
            data_ = (data_type *)realloc(
                data_,
                BITS2BLOCKS(data_size_ + increase_width) * sizeof(data_type));

            memset((char *)data_ + BITS2BLOCKS(data_size_) * sizeof(data_type),
                   0,
                   (BITS2BLOCKS(data_size_ + increase_width) -
                    BITS2BLOCKS(data_size_)) *
                       sizeof(data_type));

            data_size_ += increase_width;
        } else {
            flag_ = (data_type *)realloc(
                flag_,
                BITS2BLOCKS(flag_size_ + increase_width) * sizeof(data_type));

            memset((char *)flag_ + BITS2BLOCKS(flag_size_) * sizeof(data_type),
                   0,
                   (BITS2BLOCKS(flag_size_ + increase_width) -
                    BITS2BLOCKS(flag_size_)) *
                       sizeof(data_type));

            flag_size_ += increase_width;
        }
    }

    inline void decrease_bits(width_type decrease_width, bool is_on_data) {

        if (is_on_data) {
            data_ = (data_type *)realloc(
                data_,
                BITS2BLOCKS(data_size_ - decrease_width) * sizeof(data_type));
            data_size_ -= decrease_width;
        } else {
            flag_ = (data_type *)realloc(
                flag_,
                BITS2BLOCKS(flag_size_ - decrease_width) * sizeof(data_type));
            flag_size_ -= decrease_width;
        }
    }

    inline void keep_bits(width_type new_size, bool is_on_data) {

        if (is_on_data) {
            data_ = (data_type *)realloc(data_, BITS2BLOCKS(new_size) *
                                                    sizeof(data_type));
            data_size_ = new_size;
        } else {
            flag_ = (data_type *)realloc(flag_, BITS2BLOCKS(new_size) *
                                                    sizeof(data_type));
            flag_size_ = new_size;
        }
    }

    // count the number of set bits in a range, starting from pos with width
    inline uint64_t popcount(pos_type pos, width_type width, bool is_on_data) {

        if (width <= 64) {
            return __builtin_popcountll(GetValPos(pos, width, is_on_data));
        }

        pos_type s_off = pos % 64;
        pos_type s_idx = pos / 64;
        uint64_t count =
            __builtin_popcountll(GetValPos(pos, 64 - s_off, is_on_data));
        width -= 64 - s_off;
        s_idx += 1;

        if (is_on_data) {
            while (width > 64) {
                count += __builtin_popcountll(data_[s_idx]);
                width -= 64;
                s_idx += 1;
            }
        } else {
            while (width > 64) {
                count += __builtin_popcountll(flag_[s_idx]);
                width -= 64;
                s_idx += 1;
            }
        }
        if (width > 0) {
            return count + __builtin_popcountll(
                               GetValPos(s_idx * 64, width, is_on_data));
        }
        return count;
    }

    inline void SetValPos(pos_type pos, data_type val, width_type bits,
                          bool is_on_data) {

        pos_type s_off = pos % 64;
        pos_type s_idx = pos / 64;

        if (s_off + bits <= 64) {
            // Can be accommodated in 1 bitmap block
            if (is_on_data) {
                data_[s_idx] = (data_[s_idx] & (low_bits_set[s_off] |
                                                low_bits_unset[s_off + bits])) |
                               val << s_off;
            } else {
                flag_[s_idx] = (flag_[s_idx] & (low_bits_set[s_off] |
                                                low_bits_unset[s_off + bits])) |
                               val << s_off;
            }
        } else {
            // Must use 2 bitmap blocks
            if (is_on_data) {
                data_[s_idx] =
                    (data_[s_idx] & low_bits_set[s_off]) | val << s_off;
                data_[s_idx + 1] =
                    (data_[s_idx + 1] & low_bits_unset[(s_off + bits) % 64]) |
                    (val >> (64 - s_off));
            } else {
                flag_[s_idx] =
                    (flag_[s_idx] & low_bits_set[s_off]) | val << s_off;
                flag_[s_idx + 1] =
                    (flag_[s_idx + 1] & low_bits_unset[(s_off + bits) % 64]) |
                    (val >> (64 - s_off));
            }
        }
    }

    inline data_type GetValPos(pos_type pos, width_type bits,
                               bool is_on_data) const {

        pos_type s_off = pos % 64;
        pos_type s_idx = pos / 64;

        if (s_off + bits <= 64) {
            // Can be read from a single block
            if (is_on_data) {
                return (data_[s_idx] >> s_off) & low_bits_set[bits];
            } else {
                return (flag_[s_idx] >> s_off) & low_bits_set[bits];
            }
        } else {
            // Must be read from two blocks
            if (is_on_data) {
                return ((data_[s_idx] >> s_off) |
                        (data_[s_idx + 1] << (64 - s_off))) &
                       low_bits_set[bits];
            } else {
                return ((flag_[s_idx] >> s_off) |
                        (flag_[s_idx + 1] << (64 - s_off))) &
                       low_bits_set[bits];
            }
        }
    }

    inline void ClearWidth(pos_type pos, width_type width, bool is_on_data) {
        if (width <= 64) {
            SetValPos(pos, 0, width, is_on_data);
            return;
        }
        pos_type s_off = 64 - pos % 64;
        pos_type s_idx = pos / 64;
        SetValPos(pos, 0, s_off, is_on_data);

        width -= s_off;
        s_idx += 1;
        while (width > 64) {
            if (is_on_data)
                data_[s_idx] = 0;
            else
                flag_[s_idx] = 0;
            width -= 64;
            s_idx += 1;
        }
        SetValPos(s_idx * 64, 0, width, is_on_data);
    }

    width_type get_num_bits(pos_type node, level_t level) {

        if (is_collapse(node)) {
            return level_to_num_children[level];
        } else {
            return 1 << level_to_num_children[level];
        }
    }

    // copy from beginning to end (to lower index), so that we don't overwrite
    inline void bulkcopy_forward(pos_type from, pos_type destination,
                                 width_type bits, bool is_on_data) {
        while (bits > 64) {
            SetValPos(destination, GetValPos(from, 64, is_on_data), 64,
                      is_on_data);
            from += 64;
            destination += 64;
            bits -= 64;
        }
        SetValPos(destination, GetValPos(from, bits, is_on_data), bits,
                  is_on_data);
    }

    // copy from end to beginning (to higher index), so that we don't overwrite
    inline void bulkcopy_backward(pos_type from, pos_type destination,
                                  width_type bits, bool is_on_data) {
        while (bits > 64) {
            SetValPos(destination - 64, GetValPos(from - 64, 64, is_on_data),
                      64, is_on_data);
            bits -= 64;
            destination -= 64;
            from -= 64;
        }
        SetValPos(destination - bits, GetValPos(from - bits, bits, is_on_data),
                  bits, is_on_data);
    }

    // shift some data back and create gap
    inline void shift_backward(preorder_t node, pos_type node_pos,
                               width_type data_bits, width_type flag_bits) {

        size_type orig_data_size = data_size_;
        size_type orig_flag_size = flag_size_;

        increase_bits(data_bits, true);
        increase_bits(flag_bits, false); // not sure if we should increase bits

        bulkcopy_backward(orig_data_size, data_size_, orig_data_size - node_pos,
                          true);
        bulkcopy_backward(orig_flag_size, flag_size_, orig_flag_size - node,
                          false);

        ClearWidth(node_pos, data_bits, true);
        ClearWidth(node, flag_bits, false);
    }

    // shift data to higher index, create space to uncollapse the node
    inline void shift_backward_to_uncollapse(preorder_t from_node,
                                             pos_type from_node_pos,
                                             width_type num_children) {
        if (!is_collapse(from_node)) {
            return;
        }

        if (num_children > 20) {
            printf("num_children: %ld\n", num_children);
        }

        size_t orig_data_size = data_size_;
        width_type shift_amount = (1 << num_children) - num_children;
        increase_bits(shift_amount, true);

        bulkcopy_backward(orig_data_size, orig_data_size + shift_amount,
                          orig_data_size - from_node_pos, true);

        ClearWidth(from_node_pos, 1 << num_children, true);
        SETBITVAL(flag_, from_node);
    }

    // shift some data forward and close gap
    inline void shift_forward(preorder_t from_node, pos_type from_node_pos,
                              preorder_t to_node, pos_type to_node_pos)

    {
        bulkcopy_forward(from_node_pos, to_node_pos, data_size_ - from_node_pos,
                         true);
        bulkcopy_forward(from_node, to_node, flag_size_ - from_node, false);
        pos_type shifted_amount = from_node_pos - to_node_pos;
        ClearWidth(data_size_ - shifted_amount, shifted_amount, true);
        ClearWidth(flag_size_ - (from_node - to_node), (from_node - to_node),
                   false);

        // now decrease the allocated bits to match the new logical size
        // decrease_bits(shifted_amount, true);
        // decrease_bits((from_node - to_node), false);
    }

    inline bool is_collapse(preorder_t node) {
        if (is_collapsed_node_exp)
            return false;

        return !GETBITVAL(flag_, node);
    }

    // check if the current bitmap has a symbol
    inline bool has_symbol(preorder_t node, pos_type node_pos, morton_t symbol,
                           width_type num_children) {

        if (node >= flag_size_) {
            return false;
        }

        if (is_collapse(node)) {
            return symbol == GetValPos(node_pos, num_children, true);
        } else {
            return GETBITVAL(data_, node_pos + symbol);
        }
    }

    // get number of children of current node
    inline preorder_t get_num_children(preorder_t node, pos_type node_pos,
                                       width_type num_children) {
        if (is_collapse(node)) {
            return 1;
        } else {
            return popcount(node_pos, 1 << num_children, true);
        }
    }

    // get position for this child
    inline preorder_t get_child_skip(preorder_t node, pos_type node_pos,
                                     morton_t symbol, width_type num_children) {
        if (is_collapse(node)) {
            morton_t only_symbol = GetValPos(node_pos, num_children, true);
            if (symbol > only_symbol)
                return 1;
            return 0;
        } else {
            return popcount(node_pos, symbol, true);
        }
    }

    // get the k-th set bit in a 64-bit word
    unsigned nthset(uint64_t x, unsigned n) {
        return __builtin_ctzll(_pdep_u64(1ULL << n, x));
    }

    // get the k-th set bit in a es range
    morton_t get_k_th_set_bit(preorder_t node, unsigned k /* 0-indexed */,
                              pos_type node_pos, width_type num_children) {

        if (is_collapse(node)) {
            return GetValPos(node_pos, num_children, true);
        }
        morton_t pos_left = 1 << num_children;
        morton_t return_symbol = 0;

        while (pos_left > 64) {
            uint64_t next_block = GetValPos(node_pos + return_symbol, 64, true);
            if (next_block) {
                morton_t set_bit_count = __builtin_popcountll(next_block);
                if (k >= set_bit_count) {
                    k -= set_bit_count;
                } else if (set_bit_count > 0) {
                    return return_symbol + nthset(next_block, k);
                }
            }
            pos_left -= 64;
            return_symbol += 64;
        }

        uint64_t next_block =
            GetValPos(node_pos + return_symbol, pos_left, true);
        return return_symbol + nthset(next_block, k);
    }

    inline morton_t next_symbol(morton_t symbol, preorder_t node,
                                pos_type node_pos, morton_t end_symbol_range,
                                width_type num_children) {

        if (is_collapse(node)) {
            morton_t only_symbol = GetValPos(node_pos, num_children, true);
            if (symbol <= only_symbol)
                return only_symbol;

            return end_symbol_range + 1;
        }

        morton_t limit = end_symbol_range - symbol + 1;
        bool over_64 = false;
        if (limit > 64) {
            limit = 64;
            over_64 = true;
        }
        uint64_t next_block = GetValPos(node_pos + symbol, limit, true);
        if (next_block) {
            return __builtin_ctzll(next_block) + symbol;
        } else {
            if (over_64) {
                return next_symbol(symbol + limit, node, node_pos,
                                   end_symbol_range, num_children);
            }
            return end_symbol_range + 1;
        }
    }

    inline void shift_forward_to_collapse(preorder_t from_node,
                                          pos_type from_node_pos,
                                          width_type num_children) {
        if (is_collapse(from_node)) {
            return;
        }
        width_type shift_amount = (1 << num_children) - num_children;

        pos_type from_node_next_pos = from_node_pos + (1 << num_children);
        bulkcopy_forward(from_node_next_pos, from_node_next_pos - shift_amount,
                         data_size_ - from_node_next_pos, true);

        decrease_bits(shift_amount, true);
        ClearWidth(from_node_next_pos - num_children, num_children, true);
        CLRBITVAL(flag_, from_node);
    }

    inline void set_symbol(preorder_t node, pos_type node_pos, morton_t symbol,
                           bool was_empty, width_type num_children) {

        // if (is_collapse(node)) {
        //     if (is_collapsed_node_exp) {
        //         std::cout << "wrong! set symbol";
        //         exit(-1);
        //     }
        // }

        if (is_collapse(node)) {
            if (!was_empty) {
                morton_t only_symbol = GetValPos(node_pos, num_children, true);
                if (symbol == only_symbol) {
                    return;
                }
                shift_backward_to_uncollapse(node, node_pos, num_children);

                SETBITVAL(data_, node_pos + only_symbol);
                SETBITVAL(data_, node_pos + symbol);
                SETBITVAL(flag_, node);
            } else {
                SetValPos(node_pos, symbol, num_children, true);
            }
        } else {
            SETBITVAL(data_, node_pos + symbol);
        }
    }

    inline void copy_node_cod(compressed_bitmap *to_dfuds, preorder_t from_node,
                              pos_type from_node_pos, preorder_t to_node,
                              pos_type to_node_pos, width_type num_children) {

        width_type width;

        if (is_collapse(from_node)) {
            if (to_node == 0) {
                to_dfuds->shift_forward_to_collapse(to_node, to_node_pos,
                                                    num_children);
                CLRBITVAL(to_dfuds->flag_, to_node);
            }
            width = num_children;
        } else {
            width = 1 << num_children;
            to_dfuds->shift_backward_to_uncollapse(to_node, to_node_pos,
                                                   num_children);
            SETBITVAL(to_dfuds->flag_, to_node);
        }
        morton_t visited = 0;
        while (visited < width) {
            if (width - visited > 64) {
                to_dfuds->SetValPos(
                    to_node_pos + visited,
                    GetValPos(from_node_pos + visited, 64, true), 64, true);
                visited += 64;
            } else {
                morton_t left = width - visited;
                to_dfuds->SetValPos(
                    to_node_pos + visited,
                    GetValPos(from_node_pos + visited, left, true), left, true);
                break;
            }
        }
    }

    inline void bulk_clear_node(preorder_t start_node, pos_type start_node_pos,
                                preorder_t end_node, preorder_t end_node_pos) {
        ClearWidth(start_node_pos, end_node_pos - start_node_pos, true);
        ClearWidth(start_node, end_node + 1 - start_node, false);
    }

    size_type get_flag_size() { return flag_size_; }
    size_type get_data_size() { return data_size_; }

    // the current start is `current_offset`, just need to write there
    void serialize(FILE *file) {
        // good old start
        pointers_to_offsets_map.insert({(uint64_t)(this), current_offset});

        current_offset += sizeof(compressed_bitmap);

        compressed_bitmap *temp_bitmap =
            (compressed_bitmap *)malloc(sizeof(compressed_bitmap));
        memcpy(temp_bitmap, this, sizeof(compressed_bitmap));

        // there's no way the data_ and flag_ are already in the
        // pointers_to_offsets_map
        if (data_)
            assert(pointers_to_offsets_map.find((uint64_t)data_) ==
                   pointers_to_offsets_map.end());

        if (flag_)
            assert(pointers_to_offsets_map.find((uint64_t)flag_) ==
                   pointers_to_offsets_map.end());

        // assert that data_ won't be null when data_size_ > 0 and vice versa
        // for flag_ and flaog_size_
        assert((data_size_ == 0) == (data_ == nullptr));
        assert((flag_size_ == 0) == (flag_ == nullptr));

        // I am writing contiguous any way
        //     | compressed_bitmap | data_ | flag_ |

        // serialize the data
        if (data_size_ > 0) {
            pointers_to_offsets_map.insert({(uint64_t)data_, current_offset});

            temp_bitmap->data_ = (data_type *)(current_offset);
            current_offset += BITS2BLOCKS(data_size_) * sizeof(data_type);
        }

        // serialize the flag
        if (flag_size_ > 0) {
            pointers_to_offsets_map.insert({(uint64_t)flag_, current_offset});

            temp_bitmap->flag_ = (data_type *)(current_offset);
            current_offset += BITS2BLOCKS(flag_size_) * sizeof(data_type);
        }

        // write everything
        fwrite(temp_bitmap, sizeof(compressed_bitmap), 1, file);
        free(temp_bitmap);

        if (data_size_ > 0) {
            fwrite(data_, sizeof(data_type), BITS2BLOCKS(data_size_), file);
        }
        if (flag_size_ > 0) {
            fwrite(flag_, sizeof(data_type), BITS2BLOCKS(flag_size_), file);
        }
    }

    void deserialize(uint64_t base_addr) {
        assert((data_size_ == 0) == (data_ == nullptr));
        assert((flag_size_ == 0) == (flag_ == nullptr));

        if (data_size_ > 0) {
            data_ = (data_type *)(base_addr + (uint64_t)data_);
        }

        if (flag_size_ > 0) {
            flag_ = (data_type *)(base_addr + (uint64_t)flag_);
        }
    }

    data_type *data_ = nullptr;
    data_type *flag_ = nullptr;

  protected:
    // Data members

    // storing data: collapased + uncollapsed nodes

    // storing flags: which nodes are collapsed
    size_type data_size_ = 0;
    size_type flag_size_ = 0;
};
} // namespace compressed_bitmap

#endif