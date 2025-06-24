#ifndef DISK_BITMAP_COMPACT_VECTOR_H_
#define DISK_BITMAP_COMPACT_VECTOR_H_

#include "disk_bit_vector.h"
#include "disk_delta_encoded_array.h"

#include <limits>

namespace disk_bitmap
{

  // deleted: value_reference_vector
  // deleted: vector_iterator
  // deleted: const_vector_iterator

  template <typename T, uint8_t W>
  class disk_CompactVector : public disk_BitVector
  {
  public:
    static_assert(!std::numeric_limits<T>::is_signed,
                  "Signed types cannot be used.");
    // Type definitions
    typedef typename disk_BitVector::size_type size_type;
    typedef typename disk_BitVector::width_type width_type;
    typedef typename disk_BitVector::pos_type pos_type;
    typedef int64_t tmp_pos_type;

    // typedef value_reference_vector<CompactVector<T, W>> reference;
    typedef T value_type;
    typedef ptrdiff_t difference_type;
    typedef T *pointer;
    // typedef vector_iterator<CompactVector<T, W>> iterator;
    // typedef const_vector_iterator<CompactVector<T, W>> const_iterator;
    typedef std::random_access_iterator_tag iterator_category;

    // deleted: Constructors and Destructors
    // deleted: mutators
    // deleted: operators and iterators

    width_type GetBitWidth() const { return W; }

    size_type size() const { return size_ / W; }

    bool empty() const { return size_ == 0; }

    T Get(pos_type i, uint64_t base) const { return (T)this->GetValPos(i * W, W, base); }
  };

  class disk_CompactPtrVector : disk_CompactVector<uint64_t, 44>
  {
  public:
    // Type definitions
    typedef typename disk_BitVector::size_type size_type;
    typedef typename disk_BitVector::width_type width_type;
    typedef typename disk_BitVector::pos_type pos_type;
    typedef int64_t tmp_pos_type;

    // deleted: constructors

    // Accessors, Mutators
    void *At(pos_type idx, uint64_t base)
    {
      return reinterpret_cast<void *>(disk_CompactVector<uint64_t, 44>::Get(idx, base)
                                      << 4ULL);
    }

    // deleted: mutators

    uint64_t size_overhead()
    {
      return disk_CompactVector<uint64_t, 44>::size_overhead();
    }

    size_type get_num_elements() { return num_elements_; }

  private:
    size_type num_elements_;
  };

  class CompactPrimaryVector : disk_CompactVector<uint64_t, 46>
  {
  public:
    // Type definitions
    typedef typename disk_BitVector::size_type size_type;
    typedef typename disk_BitVector::width_type width_type;
    typedef typename disk_BitVector::pos_type pos_type;
    typedef int64_t tmp_pos_type;
    const uint64_t disk_compact_pointer_vector_size_limit = 1000;

    // deleted: constructors
    // deleted: mutators

    // Accessors, Mutators
    uint64_t At(pos_type idx, uint64_t base)
    {
      return reinterpret_cast<uint64_t>(disk_CompactVector<uint64_t, 46>::Get(idx, base)
                                        << 20ULL);
    }

    uintptr_t ptr(pos_type idx, uint64_t base) { return At(idx, base) >> 0b11; }

    size_t flag(pos_type idx, uint64_t base) { return At(idx, base) & 0b11; }

    // todo: deal with std::vectors
    std::vector<uint64_t> *get_vector_pointer(pos_type idx, uint64_t base)
    {

      return (std::vector<uint64_t> *)(ptr(idx, base) << 4ULL);
    }

    bitmap::EliasGammaDeltaEncodedArray<uint64_t> *
    get_delta_encoded_array_pointer(pos_type idx, uint64_t base)
    {
      return (bitmap::EliasGammaDeltaEncodedArray<uint64_t> *)(ptr(idx, base) << 4ULL);
    }

    uint64_t get(pos_type idx, uint32_t index, uint64_t base)
    {

      if (flag(idx, base) == 0)
      {
        return (uint64_t)ptr(idx, base);
      }
      else if (flag(idx, base) == 1)
      {
        return (*get_vector_pointer(idx, base))[index];
      }
      else
      {
        return (*get_delta_encoded_array_pointer(idx, base))[index];
      }
    }

    size_t size(pos_type idx, uint64_t base)
    {

      if (flag(idx, base) == 0)
      {
        return 1;
      }
      else if (flag(idx, base) == 1)
      {
        return get_vector_pointer(idx, base)->size();
      }
      return get_delta_encoded_array_pointer(idx, base)->get_num_elements();
    }
  };

} // namespace bitmap
#endif