#ifndef DISK_BITMAP_DELTA_ENCODED_ARRAY_H_
#define DISK_BITMAP_DELTA_ENCODED_ARRAY_H_

#include "disk_bitmap.h"
#include "disk_bitmap_array.h"
#include "utils.h"
#include <vector>

#define USE_PREFIXSUM_TABLE 1

namespace disk_bitmap
{

  template <typename T, uint32_t sampling_rate = 32>
  class disk_DeltaEncodedArray
  {
  public:
    typedef size_t size_type;
    typedef size_t pos_type;
    typedef uint8_t width_type;

    // I shouldn't be constructing nor destructing this class directly, since on disk
    // DeltaEncodedArray() = default;
    // virtual ~DeltaEncodedArray() = default;

  protected:
    // Get the encoding size for an delta value
    virtual width_type EncodingSize(T delta) = 0;

    // Deleted: Encode the delta values
    // virtual void EncodeDeltas(std::vector<T> &deltas, size_type num_deltas) = 0;
    // virtual void EncodeLastDelta(T delta, pos_type pos) = 0;

    // Encode the delta encoded array
    // no modification, so no need to construct/encode
    // void Encode(std::vector<T> &elements, size_type num_elements)

    disk_UnsizedBitmapArray<T> samples_;
    disk_UnsizedBitmapArray<pos_type> delta_offsets_;
    disk_Bitmap deltas_;
    size_type num_elements_ = 0;
    T last_val_;

  private:
  };

  static struct disk_EliasGammaPrefixSum
  {
  public:
    typedef uint16_t block_type;
    // should not be constructing

    uint16_t offset(const block_type i) const
    {
      return ((prefixsum_[(i)] >> 24) & 0xFF);
    }

    uint16_t count(const block_type i) const
    {
      return ((prefixsum_[i] >> 16) & 0xFF);
    }

    uint16_t sum(const block_type i) const { return (prefixsum_[i] & 0xFFFF); }

  private:
    uint32_t prefixsum_[65536];
  } disk_elias_gamma_prefix_table;

  template <typename T, uint32_t sampling_rate = 32>
  class disk_EliasGammaDeltaEncodedArray : public disk_DeltaEncodedArray<T, sampling_rate>
  {
  public:
    typedef typename disk_DeltaEncodedArray<T>::size_type size_type;
    typedef typename disk_DeltaEncodedArray<T>::pos_type pos_type;
    typedef typename disk_DeltaEncodedArray<T>::width_type width_type;

    // using disk_DeltaEncodedArray<T>::EncodingSize;
    // using disk_DeltaEncodedArray<T>::EncodeDeltas;
    // using disk_DeltaEncodedArray<T>::EncodeLastDelta;

    // no constructor/destructor

    size_type GetNElements() { return this->num_elements_; }

    T Get(pos_type i, uint64_t base)
    {

      pos_type samples_idx = i / sampling_rate;
      pos_type delta_offsets_idx = i % sampling_rate;
      T val = this->samples_.Get(samples_idx, base);

      if (delta_offsets_idx == 0)
        return val;

      pos_type delta_offset = this->delta_offsets_.Get(samples_idx, base);
      val += PrefixSum(delta_offset, delta_offsets_idx, base);
      return val;
    }

    pos_type BinarySearchSample(int64_t val, uint64_t base)
    {

      int64_t start = 0;
      int64_t end = (this->GetNElements() - 1) / sampling_rate;
      int64_t mid, mid_val;

      while (start <= end)
      {

        mid = (start + end) / 2;
        mid_val = this->samples_.Get(mid, base);
        if (mid_val == val)
        {
          return mid;
        }
        else if (val < mid_val)
        {
          end = mid - 1;
        }
        else
        {
          start = mid + 1;
        }
      }
      return static_cast<pos_type>(std::max(end, INT64_C(0)));
    }

    bool Find(T val, uint64_t base, pos_type *found_idx = nullptr)
    {

      pos_type sample_off = BinarySearchSample(val, base);
      pos_type current_delta_offset = this->delta_offsets_.Get(sample_off, base);
      val -= this->samples_.Get(sample_off, base);

      if (val < 0)
      {
        return false;
      }

      pos_type delta_idx = 0;
      T delta_sum = 0;
      size_type delta_max = this->deltas_.GetSizeInBits();

      while (delta_sum < val && current_delta_offset < delta_max &&
             delta_idx < sampling_rate)
      {
        uint16_t block = this->deltas_.GetValPos(current_delta_offset, 16, base);
        uint16_t block_cnt = disk_elias_gamma_prefix_table.count(block);
        uint16_t block_sum = disk_elias_gamma_prefix_table.sum(block);

        if (block_cnt == 0)
        {
          // If the prefixsum table for the block returns count == 0
          // this must mean the value spans more than 16 bits
          // read this manually
          uint8_t delta_width = 0;
          while (!this->deltas_.GetBit(current_delta_offset, base))
          {
            delta_width++;
            current_delta_offset++;
          }
          current_delta_offset++;
          auto decoded_value =
              this->deltas_.GetValPos(current_delta_offset, delta_width, base) +
              (1ULL << delta_width);
          delta_sum += decoded_value;
          current_delta_offset += delta_width;
          delta_idx += 1;

          // Roll back
          if (delta_idx == sampling_rate)
          {
            delta_idx--;
            delta_sum -= decoded_value;
            break;
          }
        }
        else if (delta_sum + block_sum < val)
        {
          // If sum can be computed from the prefixsum table
          delta_sum += block_sum;
          current_delta_offset += disk_elias_gamma_prefix_table.offset(block);
          delta_idx += block_cnt;
        }
        else
        {
          // Last few values, decode them without looking up table
          T last_decoded_value = 0;
          while (delta_sum < val && current_delta_offset < delta_max &&
                 delta_idx < sampling_rate)
          {
            int delta_width = 0;
            while (!this->deltas_.GetBit(current_delta_offset, base))
            {
              delta_width++;
              current_delta_offset++;
            }
            current_delta_offset++;
            last_decoded_value =
                this->deltas_.GetValPos(current_delta_offset, delta_width, base) +
                (1ULL << delta_width);

            delta_sum += last_decoded_value;
            current_delta_offset += delta_width;
            delta_idx += 1;
          }

          // Roll back
          if (delta_idx == sampling_rate)
          {
            delta_idx--;
            delta_sum -= last_decoded_value;
            break;
          }
        }
      }

      if (found_idx)
      {
        pos_type res = sample_off * sampling_rate + delta_idx;
        *found_idx = (delta_sum <= val) ? res : res - 1;
      }
      return val == delta_sum;
    }

    size_type get_num_elements() { return this->num_elements_; }
    // deleted: mutators
    // deleted: samples/search/find

  private:
    T PrefixSum(pos_type delta_offset, pos_type until_idx, uint64_t base)
    {
      T delta_sum = 0;
      pos_type delta_idx = 0;
      pos_type current_delta_offset = delta_offset;
      while (delta_idx != until_idx)
      {
        uint16_t block = this->deltas_.GetValPos(current_delta_offset, 16, base);
        uint16_t cnt = disk_elias_gamma_prefix_table.count(block);
        if (cnt == 0)
        {
          // If the prefixsum table for the block returns count == 0
          // this must mean the value spans more than 16 bits
          // read this manually
          width_type delta_width = 0;
          while (!this->deltas_.GetBit(current_delta_offset, base))
          {
            delta_width++;
            current_delta_offset++;
          }
          current_delta_offset++;
          delta_sum +=
              this->deltas_.GetValPos(current_delta_offset, delta_width, base) +
              (1ULL << delta_width);
          current_delta_offset += delta_width;
          delta_idx += 1;
        }
        else if (delta_idx + cnt <= until_idx)
        {
          // If sum can be computed from the prefixsum table
          delta_sum += disk_elias_gamma_prefix_table.sum(block);
          current_delta_offset += disk_elias_gamma_prefix_table.offset(block);
          delta_idx += cnt;
        }
        else
        {
          // Last few values, decode them without looking up table
          while (delta_idx != until_idx)
          {
            width_type delta_width = 0;
            while (!this->deltas_.GetBit(current_delta_offset, base))
            {
              delta_width++;
              current_delta_offset++;
            }
            current_delta_offset++;
            delta_sum +=
                this->deltas_.GetValPos(current_delta_offset, delta_width, base) +
                (1ULL << delta_width);
            current_delta_offset += delta_width;
            delta_idx += 1;
          }
        }
      }
      return delta_sum;
    }
  };
} // namespace bitmap

#endif // BITMAP_DELTA_ENCODED_ARRAY_H_