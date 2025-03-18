#pragma once

#include <array>
#include <tuple>

#include <util/system/yassert.h>
#include <util/generic/array_ref.h>

namespace NOffroad {
    template <class T, size_t blockSize, size_t tupleSize>
    class TSliceRef {
    public:
        Y_FORCE_INLINE TSliceRef(T* ptr)
            : Ptr_(ptr)
        {
        }

        template<size_t index>
        Y_FORCE_INLINE TSliceRef<T, blockSize, tupleSize - index> SubSlice() const {
            return TSliceRef<T, blockSize, tupleSize - index>(Ptr_ + index * blockSize);
        }

        Y_FORCE_INLINE T& operator[](size_t index) {
            Y_ASSERT(index < tupleSize);

            return Ptr_[index * blockSize];
        }

        Y_FORCE_INLINE const T& operator[](size_t index) const {
            Y_ASSERT(index < tupleSize);

            return Ptr_[index * blockSize];
        }

        template <size_t prefix = tupleSize, class Other>
        Y_FORCE_INLINE bool CompareLess(const Other& other) const {
            return CompareLess(other, std::make_index_sequence<prefix>());
        }

        template <class Other>
        Y_FORCE_INLINE void Assign(const Other& other) {
            for (size_t i = 0; i < tupleSize; i++)
                (*this)[i] = other[i];
        }

        Y_FORCE_INLINE void Clear() {
            for (size_t i = 0; i < tupleSize; i++)
                (*this)[i] = 0;
        }

        Y_FORCE_INLINE T Mask() const {
            T result = T();
            for (size_t i = 0; i < tupleSize; i++)
                result |= (*this)[i];
            return result;
        }

    private:
        template <class Other, size_t... indices>
        Y_FORCE_INLINE bool CompareLess(const Other& other, std::index_sequence<indices...>) const {
            (void)other;
            return std::tie((*this)[indices]...) < std::tie(other[indices]...);
        }

    private:
        T* Ptr_ = nullptr;
    };

    template <class TupleStorage>
    class TShiftedTupleStorage {
    public:
        using TTupleStorage = TupleStorage;

        TShiftedTupleStorage(TTupleStorage* storage, size_t shift)
            : Storage_(storage)
            , Shift_(shift)
        {
        }

        Y_FORCE_INLINE TArrayRef<ui32> Chunk(size_t index) {
            return Storage_->Chunk(index, Shift_);
        }

        Y_FORCE_INLINE TArrayRef<ui32> Chunk(size_t index, size_t shift) {
            return Storage_->Chunk(index, Shift_ + shift);
        }

        Y_FORCE_INLINE TArrayRef<ui32> Chunk(size_t index, size_t shift, size_t size) {
            return Storage_->Chunk(index, Shift_ + shift, size);
        }

    private:
        TTupleStorage* Storage_ = nullptr;
        size_t Shift_ = 0;
    };

    template <size_t blockSize, size_t tupleSize>
    class TTupleStorage {
    public:
        enum {
            BlockSize = blockSize,
            TupleSize = tupleSize
        };

        Y_FORCE_INLINE TSliceRef<ui32, BlockSize, TupleSize> Slice(size_t index) {
            if (TupleSize == 0)
                return {nullptr};

            return {&Data_[0][index]};
        }

        Y_FORCE_INLINE TSliceRef<const ui32, BlockSize, TupleSize> Slice(size_t index) const {
            if (TupleSize == 0)
                return {nullptr};

            return {&Data_[0][index]};
        }

        Y_FORCE_INLINE TArrayRef<ui32> Chunk(size_t index) {
            return {Data_[index]};
        }

        Y_FORCE_INLINE TArrayRef<ui32> Chunk(size_t index, size_t shift) {
            return {&Data_[index][shift], BlockSize - shift};
        }

        Y_FORCE_INLINE TArrayRef<ui32> Chunk(size_t index, size_t shift, size_t size) {
            Y_ASSERT(shift + size <= BlockSize);

            return {&Data_[index][shift], size};
        }

        Y_FORCE_INLINE ui32 operator()(size_t index, size_t component) const {
            return Data_[component][index];
        }

        Y_FORCE_INLINE ui32& operator()(size_t index, size_t component) {
            return Data_[component][index];
        }

        TShiftedTupleStorage<TTupleStorage<BlockSize, TupleSize>> Shifted(size_t shift) {
            return {this, shift};
        }

        void CopyTo(TTupleStorage<BlockSize, TupleSize>& dst) const {
            memcpy(dst.Data_.data(), Data_.data(), sizeof(ui32) * BlockSize * TupleSize);
        }

        void Clear() {
            memset(&Data_, 0, sizeof(Data_));
        }

    private:
        std::array<std::array<ui32, BlockSize>, TupleSize> Data_;
    };

}
