#ifndef DISK_BITMAP_COMPRESSED_BITMAP_H_
#define DISK_BITMAP_COMPRESSED_BITMAP_H_

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

namespace disk_compressed_bitmap
{

#define GETBIT(n, i) ((n >> i) & 1UL)
#define SETBIT(n, i) n = (n | (1UL << i))
#define CLRBIT(n, i) n = (n & ~(1UL << i))

#define BITS2BLOCKS(bits) \
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

  class disk_compressed_bitmap
  {
  public:
    typedef uint64_t pos_type;
    typedef uint64_t size_type;
    typedef uint64_t data_type;
    typedef uint64_t width_type;

    // deleted: Constructors, Destructors
    // deleted: Mutators
    // deleted: size

    inline uint64_t popcount(pos_type pos, width_type width, bool is_on_data, uint64_t base)
    {

      if (width <= 64)
      {
        return __builtin_popcountll(GetValPos(pos, width, is_on_data, base));
      }

      pos_type s_off = pos % 64;
      pos_type s_idx = pos / 64;
      uint64_t count =
          __builtin_popcountll(GetValPos(pos, 64 - s_off, is_on_data, base));
      width -= 64 - s_off;
      s_idx += 1;

      if (is_on_data)
      {
        while (width > 64)
        {
          count += __builtin_popcountll(((data_type *)((uint64_t)disk_data_ + base))[s_idx]);
          width -= 64;
          s_idx += 1;
        }
      }
      else
      {
        while (width > 64)
        {
          count += __builtin_popcountll(((data_type *)((uint64_t)disk_flag_ + base))[s_idx]);
          width -= 64;
          s_idx += 1;
        }
      }
      if (width > 0)
      {
        return count +
               __builtin_popcountll(GetValPos(s_idx * 64, width, is_on_data, base));
      }
      return count;
    }

    inline data_type GetValPos(pos_type pos,
                               width_type bits,
                               bool is_on_data,
                               uint64_t base) const
    {

      pos_type s_off = pos % 64;
      pos_type s_idx = pos / 64;

      if (s_off + bits <= 64)
      {
        // Can be read from a single block
        if (is_on_data)
        {
          return (((data_type *)((uint64_t)disk_data_ + base))[s_idx] >> s_off) & low_bits_set[bits];
        }
        else
        {
          return (((data_type *)((uint64_t)disk_flag_ + base))[s_idx] >> s_off) & low_bits_set[bits];
        }
      }
      else
      {
        // Must be read from two blocks
        if (is_on_data)
        {
          return ((((data_type *)((uint64_t)disk_data_ + base))[s_idx] >> s_off) | (((data_type *)((uint64_t)disk_data_ + base))[s_idx + 1] << (64 - s_off))) &
                 low_bits_set[bits];
        }
        else
        {
          return ((((data_type *)((uint64_t)disk_flag_ + base))[s_idx] >> s_off) | (((data_type *)((uint64_t)disk_flag_ + base))[s_idx + 1] << (64 - s_off))) &
                 low_bits_set[bits];
        }
      }
    }

    width_type get_num_bits(pos_type node, level_t level, uint64_t base)
    {

      if (is_collapse(node, base))
      {
        return level_to_num_children[level];
      }
      else
      {
        return 1 << level_to_num_children[level];
      }
    }

    inline bool is_collapse(preorder_t node, uint64_t base)
    {
      if (is_collapsed_node_exp)
        return false;

      return !GETBITVAL(((data_type *)((uint64_t)disk_flag_ + base)), node);
    }

    inline bool has_symbol(preorder_t node,
                           pos_type node_pos,
                           morton_t symbol,
                           width_type num_children, uint64_t base)
    {

      if (node >= flag_size_)
      {
        return false;
      }

      if (is_collapse(node, base))
      {
        return symbol == GetValPos(node_pos, num_children, true, base);
      }
      else
      {
        return GETBITVAL(((data_type *)((uint64_t)disk_data_ + base)), node_pos + symbol);
      }
    }

    inline preorder_t get_num_children(preorder_t node,
                                       pos_type node_pos,
                                       width_type num_children, uint64_t base)
    {
      if (is_collapse(node, base))
      {
        return 1;
      }
      else
      {
        return popcount(node_pos, 1 << num_children, true, base);
      }
    }

    inline preorder_t get_child_skip(preorder_t node,
                                     pos_type node_pos,
                                     morton_t symbol,
                                     width_type num_children, uint64_t base)
    {
      if (is_collapse(node, base))
      {
        morton_t only_symbol = GetValPos(node_pos, num_children, true, base);
        if (symbol > only_symbol)
          return 1;
        return 0;
      }
      else
      {
        return popcount(node_pos, symbol, true, base);
      }
    }

    unsigned nthset(uint64_t x, unsigned n)
    {
      return __builtin_ctzll(_pdep_u64(1ULL << n, x));
    }

    morton_t get_k_th_set_bit(preorder_t node,
                              unsigned k /* 0-indexed */,
                              pos_type node_pos,
                              width_type num_children, uint64_t base)
    {

      if (is_collapse(node, base))
      {
        return GetValPos(node_pos, num_children, true, base);
      }
      morton_t pos_left = 1 << num_children;
      morton_t return_symbol = 0;

      while (pos_left > 64)
      {
        uint64_t next_block = GetValPos(node_pos + return_symbol, 64, true, base);
        if (next_block)
        {
          morton_t set_bit_count = __builtin_popcountll(next_block);
          if (k >= set_bit_count)
          {
            k -= set_bit_count;
          }
          else if (set_bit_count > 0)
          {
            return return_symbol + nthset(next_block, k);
          }
        }
        pos_left -= 64;
        return_symbol += 64;
      }

      uint64_t next_block = GetValPos(node_pos + return_symbol, pos_left, true, base);
      return return_symbol + nthset(next_block, k);
    }

    inline morton_t next_symbol(morton_t symbol,
                                preorder_t node,
                                pos_type node_pos,
                                morton_t end_symbol_range,
                                width_type num_children,
                                uint64_t base)
    {

      if (is_collapse(node, base))
      {
        morton_t only_symbol = GetValPos(node_pos, num_children, true, base);
        if (symbol <= only_symbol)
          return only_symbol;

        return end_symbol_range + 1;
      }

      morton_t limit = end_symbol_range - symbol + 1;
      bool over_64 = false;
      if (limit > 64)
      {
        limit = 64;
        over_64 = true;
      }
      uint64_t next_block = GetValPos(node_pos + symbol, limit, true, base);
      if (next_block)
      {
        return __builtin_ctzll(next_block) + symbol;
      }
      else
      {
        if (over_64)
        {
          return next_symbol(
              symbol + limit, node, node_pos, end_symbol_range, num_children, base);
        }
        return end_symbol_range + 1;
      }
    }

    size_type get_flag_size()
    {
      return flag_size_;
    }
    size_type get_data_size() { return data_size_; }

  protected:
    // Data members
    data_type *disk_data_;
    data_type *disk_flag_;
    size_type data_size_;
    size_type flag_size_;
  };
} // namespace compressed_bitmap

#endif