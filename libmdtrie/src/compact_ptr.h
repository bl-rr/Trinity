#ifndef COMPACT_PTR_H
#define COMPACT_PTR_H

#include "delta_encoded_array.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include "disk_std_vector.h"

// Disable in most cases.
const uint64_t compact_pointer_vector_size_limit = 1000000;

namespace bits
{

  class compact_ptr
  {
  public:
    compact_ptr(n_leaves_t primary_key)
    {

      ptr_ = (uintptr_t)primary_key;
      flag_ = 0;
    }

    compact_ptr() {}

    std::vector<n_leaves_t> *get_vector_pointer()
    {
      return (std::vector<n_leaves_t> *)(ptr_ << 4ULL);
    }

    bitmap::EliasGammaDeltaEncodedArray<n_leaves_t> *
    get_delta_encoded_array_pointer()
    {
      return (bitmap::EliasGammaDeltaEncodedArray<n_leaves_t> *)(ptr_ << 4ULL);
    }

    bool scan_if_present(std::vector<n_leaves_t> *vect, n_leaves_t primary_key)
    {
      int vect_size = vect->size();

      for (int i = 0; i < vect_size; i++)
      {
        if ((*vect)[i] == primary_key)
          return true;
      }
      return false;
    }

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

    uint64_t size_overhead()
    {

      if (flag_ == 0)
      {
        return sizeof(compact_ptr);
      }
      if (flag_ == 1)
      {
        std::vector<n_leaves_t> *vect_ptr = get_vector_pointer();
        return sizeof(*vect_ptr) + sizeof(n_leaves_t) /*primary key size*/ * vect_ptr->size() + sizeof(compact_ptr);
      }
      else
      {
        return get_delta_encoded_array_pointer()
                   ->size_overhead() +
               sizeof(compact_ptr);
      }
    }

    void push(n_leaves_t primary_key)
    {

      if (flag_ == 0)
      {
        auto array = new std::vector<n_leaves_t>;
        array->push_back((n_leaves_t)ptr_);
        array->push_back(primary_key);
        ptr_ = ((uintptr_t)array) >> 4ULL;
        flag_ = 1;
        return;
      }
      else if (size() == compact_pointer_vector_size_limit + 1)
      {

        std::vector<n_leaves_t> *vect_ptr = get_vector_pointer();

        auto enc_array = new bitmap::EliasGammaDeltaEncodedArray<n_leaves_t>(
            *vect_ptr, vect_ptr->size());
        delete vect_ptr;
        ptr_ = ((uintptr_t)enc_array) >> 4ULL;
        flag_ = 2;
      }
      if (flag_ == 1)
      {
        get_vector_pointer()->push_back(primary_key);
      }
      else
      {
        get_delta_encoded_array_pointer()->Push(primary_key);
      }
    }

    uint64_t get(n_leaves_t index)
    {
      if (index >= size())
        return 0;

      if (flag_ == 0)
      {
        return (uint64_t)ptr_;
      }
      if (flag_ == 1)
      {
        return (*get_vector_pointer())[index];
      }
      else
      {
        return (*get_delta_encoded_array_pointer())[index];
      }
    }

    bool check_if_present(n_leaves_t primary_key)
    {

      if (flag_ == 0)
      {
        return primary_key == (uint64_t)ptr_;
      }
      else if (flag_ == 1)
      {
        return binary_if_present(get_vector_pointer(), primary_key);
      }
      else
      {
        return get_delta_encoded_array_pointer()->Find(primary_key);
      }
    }

    size_t size()
    {

      if (flag_ == 0)
      {
        return 1;
      }
      else if (flag_ == 1)
      {
        return get_vector_pointer()->size();
      }
      return get_delta_encoded_array_pointer()->get_num_elements();
    }

    void serialize(FILE *file, uint64_t index, bits::compact_ptr *data)
    {
      // each of this already has a place in the file, as specified by data
      if (flag_ == 0)
      {
        // the data is on the ptr
        data[index].ptr_ = ptr_;
        data[index].flag_ = flag_;
      }
      else if (flag_ == 1)
      {
        // Write the vector pointer
        std::vector<n_leaves_t> *vect_ptr = get_vector_pointer();
        disk_std_vector<n_leaves_t> temp_vect = disk_std_vector<n_leaves_t>::allocate_space(*vect_ptr);

        uint64_t disk_std_vector_offset = current_offset; // in place of ptr_ later
        current_offset += sizeof(disk_std_vector<n_leaves_t>);

        uint64_t disk_std_vector_data_offset = current_offset; // in the disk_std_vector
        n_leaves_t *disk_std_vector_data_ptr = temp_vect.disk_data_;
        temp_vect.disk_data_ = (n_leaves_t *)disk_std_vector_data_offset;

        current_offset += vect_ptr->size() * sizeof(n_leaves_t);

        for (n_leaves_t i = 0; i < vect_ptr->size(); i++)
        {
          disk_std_vector_data_ptr[i] = (*vect_ptr)[i];
        }

        // write the disk_std_vector and the data
        if (ftell(file) != disk_std_vector_offset)
        {
          fseek(file, disk_std_vector_offset, SEEK_SET);
          fwrite(&temp_vect, sizeof(disk_std_vector<n_leaves_t>), 1, file);
          fseek(file, 0, SEEK_END);
        }
        else
        {
          fwrite(&temp_vect, sizeof(disk_std_vector<n_leaves_t>), 1, file);
        }

        if (ftell(file) != disk_std_vector_data_offset)
        {
          fseek(file, disk_std_vector_data_offset, SEEK_SET);
          fwrite(disk_std_vector_data_ptr, sizeof(n_leaves_t),
                 vect_ptr->size(), file);
          fseek(file, 0, SEEK_END);
        }
        else
        {
          fwrite(disk_std_vector_data_ptr, sizeof(n_leaves_t),
                 vect_ptr->size(), file);
        }
        // assert disk_std_vector_offset is divisible by 16
        // assert(disk_std_vector_offset % 16 == 0);
        data[index].ptr_ = disk_std_vector_offset;
        data[index].flag_ = 1;

        free(disk_std_vector_data_ptr);
      }
      else
      {
        // assert delta_array_offset % 16 == 0
        // assert(current_offset % 16 == 0);
        data[index].ptr_ = (uintptr_t)(current_offset);
        data[index].flag_ = 2;
        get_delta_encoded_array_pointer()->serialize(file);
      }
    }

  private:
    uintptr_t ptr_ : 44;
    unsigned flag_ : 2;
  } __attribute__((packed));

} // namespace bits

#endif // COMPACT_PTR_H