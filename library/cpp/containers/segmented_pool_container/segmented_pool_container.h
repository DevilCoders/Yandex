#pragma once

#include <util/generic/noncopyable.h>
#include <util/memory/segmented_string_pool.h>
#include <util/system/yassert.h>

#include <type_traits>

template <class T, class Alloc = std::allocator<T>>
class segmented_pool_container: protected  segmented_pool<T, Alloc> {
private:
    // DEBUG
    ui32 TotalElements;

    void uninitialized_init_n(T* first, const size_t count) noexcept(std::is_nothrow_constructible<T>::value) {
        T* const end = first + count;
        while (first < end) {
            new (first++) T();
        }
    }

    void raw_destroy_n(T* first, const size_t count) noexcept(std::is_nothrow_destructible<T>::value) {
        T* const end = first + count;
        while (first < end) {
            (first++)->~T();
        }
    }

public:
    explicit segmented_pool_container(size_t segsz = 1024 * 1024, const char* name = nullptr)
        : segmented_pool<T, Alloc>(segsz, name)
    {
    }

    ~segmented_pool_container() {
        clear();
    }

    /* src - array of objects, len - count of elements in array */
    T* append(const T* src, size_t len) {
        // DEBUG
        TotalElements += len;

        segmented_pool<T, Alloc>::check_capacity(len);
        ui8* rv = (ui8*)segmented_pool<T, Alloc>::curseg->data + segmented_pool<T, Alloc>::curseg->freepos;
        segmented_pool<T, Alloc>::last_ins_size = sizeof(T) * len;
        if (src)
            std::uninitialized_copy(src, src + len, (T*)rv);
        else
            uninitialized_init_n((T*)rv, len);

        segmented_pool<T, Alloc>::curseg->freepos += segmented_pool<T, Alloc>::last_ins_size,
            segmented_pool<T, Alloc>::last_free -= len;
        return (T*)rv;
    }
    T* append() {
        T* rv = segmented_pool<T, Alloc>::get_raw();
        try {
            new (rv) T();
        } catch (...) {
            segmented_pool<T, Alloc>::undo_last_append();
            throw;
        }

        return rv;
    }
    T* reset_last() {
        Y_ASSERT((segmented_pool<T, Alloc>::curseg != segmented_pool<T, Alloc>::segs.end())); // do not use before append()
        Y_ASSERT((segmented_pool<T, Alloc>::last_ins_size == sizeof(T)));                     // works for only one element

        T* last_element = (T*)(segmented_pool<T, Alloc>::curseg->freepos - segmented_pool<T, Alloc>::last_ins_size);
        last_element->~T();
        try {
            new (last_element) T();
        } catch (...) {
            segmented_pool<T, Alloc>::undo_last_append();
            throw;
        }
        return last_element;
    }

    void restart() {
        for (typename segmented_pool<T, Alloc>::seg_iterator i = segmented_pool<T, Alloc>::segs.begin();
             i != segmented_pool<T, Alloc>::segs.end();
             ++i) {
            raw_destroy_n(i->data, i->freepos / sizeof(T));
            i->freepos = 0;
        }
        segmented_pool<T, Alloc>::curseg = segmented_pool<T, Alloc>::segs.begin();
        segmented_pool<T, Alloc>::last_free = 0;
        segmented_pool<T, Alloc>::last_ins_size = 0;
    }
    void clear() {
        for (typename segmented_pool<T, Alloc>::seg_iterator i = segmented_pool<T, Alloc>::segs.begin();
             i != segmented_pool<T, Alloc>::segs.end();
             ++i) {
            raw_destroy_n(i->data, i->freepos / sizeof(T));
            segmented_pool<T, Alloc>::seg_allocator.deallocate(i->data, i->_size);
        }
        segmented_pool<T, Alloc>::segs.clear();
        segmented_pool<T, Alloc>::curseg = segmented_pool<T, Alloc>::segs.begin();
        segmented_pool<T, Alloc>::last_free = 0;
        segmented_pool<T, Alloc>::last_ins_size = 0;
    }
    void undo_last_append() {
        Y_ASSERT((segmented_pool<T, Alloc>::curseg != segmented_pool<T, Alloc>::segs.end())); // do not use before append()
        if (segmented_pool<T, Alloc>::last_ins_size) {
            Y_ASSERT((segmented_pool<T, Alloc>::last_ins_size <= segmented_pool<T, Alloc>::curseg->freepos));
            segmented_pool<T, Alloc>::curseg->freepos -= segmented_pool<T, Alloc>::last_ins_size;
            raw_destroy_n((T*)segmented_pool<T, Alloc>::curseg->freepos, segmented_pool<T, Alloc>::last_ins_size / sizeof(T));
            segmented_pool<T, Alloc>::last_free += segmented_pool<T, Alloc>::last_ins_size / sizeof(T);
            segmented_pool<T, Alloc>::last_ins_size = 0;
        }
    }

    // iterator

    class iterator {
    private:
        friend class segmented_pool_container;

        segmented_pool_container* Container;
        size_t SegNo;
        size_t SegOffset;

    protected:
        iterator(segmented_pool_container* containter, size_t segNo, size_t segOffset)
            : Container(containter)
            , SegNo(segNo)
            , SegOffset(segOffset)
        {
        }

    public:
        iterator& operator++() {
            typename segmented_pool<T, Alloc>::seg_container& segs = Container->segmented_pool<T, Alloc>::segs;
            Y_ASSERT(SegNo < segs.size());
            ++SegOffset;
            if (segs[SegNo].freepos / sizeof(T) == SegOffset) {
                SegOffset = 0;
                do {
                    ++SegNo;
                } while ((SegNo < segs.size()) && (segs[SegNo].freepos == 0));
            }
            return *this;
        }

        iterator operator++(int) {
            iterator it(*this);
            ++(*this);
            return it;
        }

        T& operator*() {
            Y_ASSERT((SegNo < Container->segmented_pool<T, Alloc>::segs.size()));
            Y_ASSERT((SegOffset < (Container->segmented_pool<T, Alloc>::segs[SegNo].freepos / sizeof(T))));
            return Container->segmented_pool<T, Alloc>::segs[SegNo].data[SegOffset];
        }

        T* operator->() {
            Y_ASSERT((SegNo < Container->segmented_pool<T, Alloc>::segs.size()));
            Y_ASSERT((SegOffset < (Container->segmented_pool<T, Alloc>::segs[SegNo].freepos / sizeof(T))));
            return Container->segmented_pool<T, Alloc>::segs[SegNo].data + SegOffset;
        }

        bool operator==(const iterator& rhs) const {
            Y_ASSERT(Container == rhs.Container);

            return (SegNo == rhs.SegNo) && (SegOffset == rhs.SegOffset);
        }

        bool operator!=(const iterator& rhs) const {
            Y_ASSERT(Container == rhs.Container);

            return (SegNo != rhs.SegNo) || (SegOffset != rhs.SegOffset);
        }
    };

    iterator begin() {
        size_t segNo = 0;

        while ((segNo < segmented_pool<T, Alloc>::segs.size()) &&
               (segmented_pool<T, Alloc>::segs[segNo].freepos == 0))
            ++segNo;

        return iterator(this, segNo, 0);
    }

    iterator end() {
        return iterator(this, segmented_pool<T, Alloc>::segs.size(), 0);
    }
};
