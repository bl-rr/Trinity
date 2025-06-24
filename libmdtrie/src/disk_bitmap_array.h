#ifndef DISK_BITMAP_BITMAP_ARRAY_H_
#define DISK_BITMAP_BITMAP_ARRAY_H_

#include "disk_bitmap.h"

#include <limits>

namespace disk_bitmap
{

  // deleted value_reference
  // deleted bit_map_array_iterator
  // deleted const_bitmap_array_iterator

  template <typename T>
  class disk_BitmapArray : public disk_Bitmap
  {
  public:
    // deleted: Constructors and destructors

    // Getters
    size_type GetNumElements() { return num_elements_; }

    width_type GetBitWidth() { return bit_width_; }

    size_type size() const { return num_elements_; }

    size_type max_size() const { return num_elements_; }

    bool empty() const { return num_elements_ == 0; }

  protected:
    // Data members
    size_type num_elements_;
    width_type bit_width_;
  };

  // Unsigned bitmap array that does not store number of elements in order to
  // save space; does not provide iterators as a consequence. Access/Modify with
  // care, internal bound checks may not be possible
  template <typename T, uint8_t bit_width_ = 32>
  class disk_UnsizedBitmapArray : public disk_Bitmap
  {
  public:
    // Type definitions
    typedef typename disk_BitmapArray<T>::size_type size_type;
    typedef typename disk_BitmapArray<T>::width_type width_type;
    typedef typename disk_BitmapArray<T>::pos_type pos_type;

    // typedef value_reference<UnsizedBitmapArray<T>> reference;
    typedef T value_type;

    // deleted: Constructors and destructors
    // deleted: mutators

    T Get(pos_type i, uint64_t base) const
    {
      return (T)this->GetValPos(i * bit_width_, bit_width_, base);
    }

    // deleted: operators and iterators

  private:
  };

  template <typename T>
  class disk_UnsignedBitmapArray : public disk_BitmapArray<T>
  {
  public:
    static_assert(!std::numeric_limits<T>::is_signed,
                  "Signed types cannot be used with UnsignedBitmapArray.");

    // Type definitions
    typedef typename disk_BitmapArray<T>::size_type size_type;
    typedef typename disk_BitmapArray<T>::width_type width_type;
    typedef typename disk_BitmapArray<T>::pos_type pos_type;

    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T *pointer;
    // typedef value_reference<UnsignedBitmapArray<T>> reference;
    // typedef bitmap_array_iterator<UnsignedBitmapArray<T>> iterator;
    // typedef const_bitmap_array_iterator<UnsignedBitmapArray<T>> const_iterator;
    typedef std::random_access_iterator_tag iterator_category;

    // deleted: constructors and destructors
    // deleted: mutators

    T Get(pos_type i, uint64_t base) const
    {
      return (T)this->GetValPos(i * this->bit_width_, this->bit_width_, base);
    }

    // deleted: Operators, iterators

    // void swap(const disk_UnsignedBitmapArray<T> &other)
    // {
    //   using std::swap;
    //   swap(this->data_, other.data_);
    //   swap(this->size_, other.size_);
    //   swap(this->num_elements_, other.num_elements_);
    //   swap(this->bit_width_, other.bit_width_);
    // }
  };

  template <typename T>
  class disk_SignedBitmapArray : public disk_BitmapArray<T>
  {
  public:
    static_assert(std::numeric_limits<T>::is_signed,
                  "Unsigned types should not be used with SignedBitmapArray.");

    // Type definitions
    typedef typename disk_BitmapArray<T>::size_type size_type;
    typedef typename disk_BitmapArray<T>::width_type width_type;
    typedef typename disk_BitmapArray<T>::pos_type pos_type;

    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T *pointer;
    // typedef value_reference<SignedBitmapArray<T>> reference;
    // typedef bitmap_array_iterator<SignedBitmapArray<T>> iterator;
    // typedef const_bitmap_array_iterator<SignedBitmapArray<T>> const_iterator;
    typedef std::random_access_iterator_tag iterator_category;

    // deleted: Constructors and destructors

    // deleted: mutators
    T Get(pos_type i, uint64_t base) const
    {
      T value = this->GetValPos(i * this->bit_width_, this->bit_width_, base);
      bool negate = (value & 1);
      return ((value >> 1) ^ -negate) + negate;
    }

    // deleted: Operators, iterators

    // void swap(const SignedBitmapArray<T> &other)
    // {
    //   using std::swap;
    //   swap(this->data_, other.data_);
    //   swap(this->size_, other.size_);
    //   swap(this->num_elements_, other.num_elements_);
    //   swap(this->bit_width_, other.bit_width_);
    //   swap(this->signs_, other.signs_);
    // }
  };

} // namespace bitmap
#endif