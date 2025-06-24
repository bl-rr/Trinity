#ifndef DISK_COMPACT_PTR_H
#define DISK_COMPACT_PTR_H

#include "disk_delta_encoded_array.h"
#include <cstddef>
#include <cstdint>
#include <vector>

// Disable in most cases.
const uint64_t disk_compact_pointer_vector_size_limit = 1000000;

namespace disk_bits
{

  class disk_compact_ptr
  {
  public:
    // deleted: constructors
    // todo: deal with std::vectors
    std::vector<n_leaves_t> *get_vector_pointer(uint64_t base)
    {
      return (std::vector<n_leaves_t> *)(disk_ptr_ << 4ULL + base);
    }

    disk_bitmap::disk_EliasGammaDeltaEncodedArray<n_leaves_t> *
    get_delta_encoded_array_pointer(uint64_t base)
    {
      return (disk_bitmap::disk_EliasGammaDeltaEncodedArray<n_leaves_t> *)(disk_ptr_ << 4ULL + base);
    }

    // deleted: checkers + size
    // deleted: mutators

    bool binary_if_present(std::vector<n_leaves_t> *vect, n_leaves_t primary_key)
    {
      uint64_t low = 0;
      uint64_t high = vect->size() - 1;

      while (low + 1 < high)
      {
        uint64_t mid = (low + high) / 2;
        if ((*vect)[mid] < primary_key)
        {
          low = mid;
        }
        else
        {
          high = mid;
        }
      }
      if ((*vect)[low] == primary_key || (*vect)[high] == primary_key)
      {
        return true;
      }
      return false;
    }

    uint64_t get(n_leaves_t index, uint64_t base)
    {
      if (index >= size(base))
        return 0;

      if (flag_ == 0)
      {
        return (uint64_t)(disk_ptr_ + base);
      }
      if (flag_ == 1)
      {
        return (*get_vector_pointer(base))[index];
      }
      else
      {
        return (*get_delta_encoded_array_pointer(base)).Get(index, base);
      }
    }

    bool check_if_present(n_leaves_t primary_key, uint64_t base)
    {

      if (flag_ == 0)
      {
        return primary_key == (uint64_t)disk_ptr_ + base;
      }
      else if (flag_ == 1)
      {
        return binary_if_present(get_vector_pointer(base), primary_key);
      }
      else
      {
        return get_delta_encoded_array_pointer(base)->Find(primary_key, base);
      }
    }

    size_t size(uint64_t base)
    {

      if (flag_ == 0)
      {
        return 1;
      }
      else if (flag_ == 1)
      {
        return get_vector_pointer(base)->size();
      }
      return get_delta_encoded_array_pointer(base)->get_num_elements();
    }

  private:
    uintptr_t disk_ptr_ : 44;
    unsigned flag_ : 2;
  } __attribute__((packed));

} // namespace bits

#endif // COMPACT_PTR_H