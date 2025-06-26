#ifndef DISK_BITMAP_BIT_VECTOR_H_
#define DISK_BITMAP_BIT_VECTOR_H_

#include "utils.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace bitmap
{
  class CompactPtrVector;
}

namespace disk_bitmap
{

  class disk_BitVector
  {
  public:
    typedef size_t pos_type;
    typedef size_t size_type;
    typedef uint64_t data_type;
    typedef uint8_t width_type;

    // deleted: constructors and destructors
    // deleted: mutators

    // Getters
    data_type *GetData(uint64_t base) { return (data_type *)((uint64_t)disk_data_ + base); }

    size_type GetSizeInBits() const { return size_; }

    uint64_t size_overhead() const
    {

      return sizeof(size_) + sizeof(data_type *) +
             sizeof(data_type) * BITS2BLOCKS(size_);
    }

    bool GetBit(pos_type i, uint64_t base) const { return GETBITVAL((data_type *)((uint64_t)disk_data_ + base), i); }

    data_type GetValPos(pos_type pos, width_type bits, uint64_t base) const
    {
      pos_type s_off = pos % 64;
      pos_type s_idx = pos / 64;

      if (s_off + bits <= 64)
      {
        // Can be read from a single block
        return (((data_type *)((uint64_t)disk_data_ + base))[s_idx] >> s_off) & low_bits_set[bits];
      }
      else
      {
        // Must be read from two blocks
        return ((((data_type *)((uint64_t)disk_data_ + base))[s_idx] >> s_off) | (((data_type *)((uint64_t)disk_data_ + base))[s_idx + 1] << (64 - s_off))) &
               low_bits_set[bits];
      }
    }

  protected:
    // Data members
    // uint8_t _padding[8]; // Add 8 bytes to match BitVector
    data_type *disk_data_{};
    size_type size_{};

    friend class bitmap::CompactPtrVector;
  };

} // namespace bitmap

#endif