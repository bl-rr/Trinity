#ifndef DISK_MD_TRIE_STD_VECTOR_H
#define DISK_MD_TRIE_STD_VECTOR_H

#include "defs.h"
#include <vector>

template <typename T>
class disk_std_vector
{
public:
    // allocate the space that the disk_std_vector will use
    static disk_std_vector allocate_space(std::vector<T> vec)
    {
        disk_std_vector new_vect;
        new_vect.size_ = vec.size();
        new_vect.capacity_ = new_vect.size_;
        new_vect.disk_data_ = (T *)malloc(new_vect.size_ * sizeof(T));
        if (!new_vect.disk_data_)
        {
            throw std::bad_alloc();
        }
        return new_vect;
    }

    T get(size_t index, uint64_t base) const
    {
        if (index >= size_)
        {
            throw std::out_of_range("Index out of range");
        }
        return ((T *)((uint64_t)disk_data_ + base))[index];
    }

    size_t size() const
    {
        return size_;
    }

    // should not have a destructor

    T *data()
    {
        return disk_data_;
    }

    T *disk_data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

#endif // DISK_MD_TRIE_STD_VECTOR_H
