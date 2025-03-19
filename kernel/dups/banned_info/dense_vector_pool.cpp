#include "dense_vector_pool.h"

#include <util/generic/hash.h>
#include <util/generic/deque.h>
#include <util/system/yassert.h>

namespace {
    template <class T>
    class TDenseVectorPool: private TNonCopyable {
        static_assert(std::is_integral<T>::value, "expected integral type");

    public:
        using TValueType = T;
        using TArrayType = TArrayRef<TValueType>;

        TDenseVectorPool(TMemoryPool* pool)
            : Pool(pool)
        {
            Y_ENSURE(pool);
        }

        TArrayType GetArray(const size_t length) {
            auto iter = FreeList.find(length);
            if (iter != FreeList.end()) {
                auto& deque = iter->second;
                Y_VERIFY_DEBUG(!deque.empty());
                TArrayType result = deque.back();
                Y_VERIFY_DEBUG(result.size() == length);
                deque.pop_back();
                if (deque.empty()) {
                    FreeList.erase(iter);
                }
                return result;
            }
            if (length == 0) {
                return TArrayType{};
            }
            TValueType* t = Pool->AllocateZeroArray<TValueType>(length);
            return TArrayType{t, t + length};
        }

        void ReleaseArray(TArrayType array) {
            if (array.size() == 0) {
                return;
            }
            auto& deque = FreeList[array.size()];
            deque.push_back(array);
        }

        void ResizeArray(TArrayType& array, const size_t newLength, TArrayType* newSlice) {
            if (Y_UNLIKELY(array.size() == newLength)) {
                if (newSlice) {
                    (*newSlice) = TArrayType{};
                }
                return;
            }
            TArrayType n = GetArray(newLength);
            for (size_t i = 0; i < Min(array.size(), newLength); ++i) {
                n[i] = std::move(array[i]);
            }
            if (newSlice) {
                if (array.size() < newLength) {
                    (*newSlice) = n.Slice(array.size());
                } else {
                    (*newSlice) = TArrayType{};
                }
            }
            ReleaseArray(array);
            array = n;
        }

        TArrayType GrowArray(TArrayType& array, const size_t appLength) {
            TArrayType newSlice;
            ResizeArray(array, array.size() + appLength, &newSlice);
            Y_VERIFY_DEBUG(newSlice.size() == appLength);
            return newSlice;
        }

        void ForgetAndFreeIntermediateData() {
            FreeList.clear();
        }

        size_t WastedItems() const {
            size_t waste = 0;
            for (auto& it : FreeList) {
                const size_t length = it.first;
                const size_t num = it.second.size();
                waste += length * num;
            }
            return waste;
        }

        size_t WastedBytes() const {
            return WastedItems() * sizeof(TValueType);
        };

    private:
        TMemoryPool* Pool;
        THashMap<size_t, TDeque<TArrayType>> FreeList;
    };

}

namespace NDups {
    class TAppendOnlyDenseVectorPool::TImpl: protected TDenseVectorPool<THashType> {
    public:
        using TBase = TDenseVectorPool<THashType>;

        using TBase::TBase;
        using TBase::GrowArray;
        using TBase::WastedItems;
        using TBase::WastedBytes;
    };

    TAppendOnlyDenseVectorPool::TAppendOnlyDenseVectorPool(TMemoryPool* pool)
        : Impl(new TImpl(pool))
    {
    }

    TAppendOnlyDenseVectorPool::TArrayType TAppendOnlyDenseVectorPool::GrowArray(TManagedArrayType& array, const size_t additionalLength) {
        return Impl->GrowArray(array, additionalLength);
    }

    size_t TAppendOnlyDenseVectorPool::WastedItems() const {
        return Impl->WastedItems();
    }

    size_t TAppendOnlyDenseVectorPool::WastedBytes() const {
        return Impl->WastedBytes();
    }

    TAppendOnlyDenseVectorPool::~TAppendOnlyDenseVectorPool() = default;
}
