#ifndef DEVICE_VECTOR_H_
#define DEVICE_VECTOR_H_

#include <cstdlib>
#include <cstring>

template <typename T> class device_vector {
  public:
    T *get_data_address() { return data; }
    size_t *get_capacity_address() { return &cap; }
    size_t *get_size_address() { return &len; }

    size_t len;
    T *data;
    size_t cap;

  private:
  public:
    void init(size_t size, size_t capacity) {
        cap = capacity;
        len = size;
    }

    void reallocate(size_t new_cap) {
        T *new_data = (T *)malloc(new_cap * sizeof(T));
        if (data) {
            memcpy(new_data, data, len * sizeof(T));
            free(data);
        }
        data = new_data;
        cap = new_cap;
    }
    explicit device_vector() : data(nullptr), cap(0), len(0) {}

    device_vector(size_t size)
        : data((T *)malloc(size * sizeof(T))), cap(size), len(size) {
        memset(data, 0, size * sizeof(T));
    }

    device_vector(size_t size, const T &value)
        : data((T *)malloc(size * sizeof(T))), cap(size), len(size) {
        for (size_t i = 0; i < size; ++i) {
            new (data + i) T(value); // Placement new
        }
    }

    device_vector(const device_vector &other)
        : data((T *)malloc(other.len * sizeof(T))), cap(other.len),
          len(other.len) {
        memcpy(data, other.data, len * sizeof(T));
    }

    device_vector(device_vector &&other) noexcept
        : data(other.data), cap(other.cap), len(other.len) {
        other.data = nullptr;
        other.cap = 0;
        other.len = 0;
    }

    virtual ~device_vector() {
        for (size_t i = 0; i < len; ++i) {
            data[i].~T(); // Explicit call to the destructor
        }
        free(data);
    }

    device_vector &operator=(const device_vector &other) {
        if (this != &other) {
            T *new_data = (T *)malloc(other.len * sizeof(T));
            memcpy(new_data, other.data, other.len * sizeof(T));
            for (size_t i = 0; i < len; ++i) {
                data[i].~T();
            }
            free(data);
            data = new_data;
            cap = other.len;
            len = other.len;
        }
        return *this;
    }

    device_vector &operator=(device_vector &&other) noexcept {
        if (this != &other) {
            for (size_t i = 0; i < len; ++i) {
                data[i].~T();
            }
            free(data);
            data = other.data;
            cap = other.cap;
            len = other.len;
            other.data = nullptr;
            other.cap = 0;
            other.len = 0;
        }
        return *this;
    }

    T &operator[](size_t index) {
        // assert(index < len);
        return data[index];
    }

    const T &operator[](size_t index) const {
        // assert(index < len);
        return data[index];
    }

    size_t size() const { return len; }

    size_t capacity() const { return cap; }

    bool empty() const { return len == 0; }

    void resize(size_t new_size, const T &value = T()) {
        if (new_size > cap) {
            reallocate(new_size);
        }
        for (size_t i = len; i < new_size; ++i) {
            new (data + i) T(value);
        }
        len = new_size;
    }

    void push_back(const T &value) {
        if (len == cap) {
            reallocate(cap == 0 ? 1 : cap * 2);
        }
        new (data + len++) T(value);
    }

    T *begin() { return data; }

    T *end() { return data + len; }

    void insert(T *position, const T &value) {
        // if (position < data || position > data + len)
        // {
        //     assert(0); // Position out of bounds
        // }
        size_t index = position - data;
        if (len == cap) {
            // printf("reallocate\n");
            reallocate(cap == 0
                           ? 1
                           : cap * 2); // Double the capacity if vector is full
        }
        // Shift elements to the right to make space for the new element
        for (int i = len; i > index; --i) {
            data[i] = data[i - 1];
        }
        new (data + index)
            T(value); // Placement new to construct the new element in place
        ++len;
    }

    void erase(T *position) {
        // if (position < data || position >= data + len)
        // {
        //     assert(0);
        // }
        size_t index = position - data;
        data[index].~T();
        // memmove
        memcpy(data + index, data + index + 1, (len - index - 1) * sizeof(T));
        --len;
    }

    void erase(T *first, T *last) {
        // if (first < data || last > data + len || first > last)
        // {
        //     assert(0);
        // }
        size_t first_index = first - data;
        size_t last_index = last - data;
        size_t num_elements = last_index - first_index;
        for (size_t i = first_index; i < last_index; ++i) {
            data[i].~T();
        }
        // memmove
        memcpy(data + first_index, data + last_index,
               (len - last_index) * sizeof(T));
        len -= num_elements;
    }
};

#endif // DEVICE_VECTOR_H_