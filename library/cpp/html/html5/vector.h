#pragma once

#include <functional>

#include <assert.h>
#include <memory.h>

namespace NHtml5 {
    /**
     * A simple vector implementation.  This stores a pointer to a data array and a
     * length.  Overflows upon addition result in reallocation of the data
     * array, with the size doubling to maintain O(1) amortized cost.  There is no
     * removal function, as this isn't needed for any of the operations within this
     * library.  Iteration can be done through inspecting the structure directly in
     * a for-loop.
     */
    template <typename T>
    struct TVectorType {
        typedef std::function<T*(size_t)> TAlloc;

        /**
         * Data elements.  This points to a dynamically-allocated array of capacity
         * elements, each a void* to the element itself.
         */
        T* Data;

        /** Number of elements currently in the vector. */
        unsigned int Length;

        /** Current array capacity. */
        unsigned int Capacity;

    public:
        // Copy range of elements to the vector.
        // Suppose vector was not been initialized.
        template <template <typename> class TRange>
        inline void Assign(const TRange<T>& range, TAlloc a) {
            if (range.Length) {
                Data = a(range.Length);
                Length = range.Length;
                Capacity = range.Length;
            } else {
                Data = nullptr;
                Length = 0;
                Capacity = 0;
                return;
            }
            for (size_t i = 0; i < range.Length; ++i) {
                Data[i] = range.Data[i];
            }
            assert(Data);
            assert(Length <= Capacity);
        }

        inline void Initialize(unsigned int capacity, TAlloc a) {
            Data = (capacity == 0) ? nullptr : a(capacity);
            Length = 0;
            Capacity = capacity;
        }

        inline void InsertAt(const T& elem, int index, TAlloc a) {
            assert(index >= 0);
            assert((unsigned int)index <= Length);
            EnlargeVectorIfFull(a);
            ++Length;
            memmove(&Data[index + 1], &Data[index], sizeof(T) * (Length - index - 1));
            Data[index] = elem;
        }

        template <template <typename> class TRange>
        inline void InsertRangeAt(const TRange<T>& range, int index, TAlloc a) {
            assert(index >= 0);
            assert((unsigned int)index <= Length);
            EnlargeVectorIfFull(a, range.Length);
            memmove(&Data[index + range.Length], &Data[index], sizeof(T) * (Length - index));
            Length += range.Length;
            for (size_t j = 0; j < range.Length; ++j) {
                Data[index + j] = range.Data[j];
            }
        }

        // Append one element to the vector.
        inline void PushBack(const T& elem, TAlloc a) {
            assert(Data != nullptr);
            EnlargeVectorIfFull(a);
            assert(Data);
            assert(Length < Capacity);
            Data[Length++] = elem;
        }

        inline void RemoveAt(int index) {
            assert(index >= 0);
            assert((unsigned int)index < Length);
            memmove(&Data[index], &Data[index + 1], sizeof(T) * (Length - index - 1));
            --Length;
        }

    private:
        void EnlargeVectorIfFull(TAlloc a, size_t size = 1) {
            size_t reserve = Length + size;
            if (reserve > Capacity) {
                if (Capacity) {
                    size_t old_num_bytes = sizeof(T) * Capacity;
                    Capacity *= 3;
                    if (reserve > Capacity)
                        Capacity = reserve;
                    T* temp = a(Capacity);
                    memcpy(temp, Data, old_num_bytes);
                    //delete [] Data;
                    Data = temp;
                } else {
                    // 0-capacity vector; no previous array to deallocate.
                    Capacity = size + 1;
                    Data = a(Capacity);
                }
            }
        }
    };

}
