#pragma once

#include <util/memory/pool.h>

template <class T>
class TMsFixedVector {
    public:
        inline TMsFixedVector()
            : Pool_(nullptr)
            , Len_(0)
            , Data_(nullptr)
        {
        }

        inline TMsFixedVector(TMemoryPool* pool, size_t len)
            : Pool_(pool)
            , Data_((T*)Pool_->Allocate(len * sizeof(T)))
        {
            try {
                for (Len_ = 0; Len_ < len; ++Len_) {
                    new (Data_ + Len_) T;
                }
            } catch (...) {
                Clear();

                throw;
            }
        }

        inline ~TMsFixedVector() {
            Clear();
        }

        inline void Clear() noexcept {
            for (size_t i = 0; i < Len_; ++i) {
                (Data_ + i)->~T();
            }

            Data_ = nullptr;
            Len_ = 0;
        }

        inline void Swap(TMsFixedVector& r) noexcept {
            DoSwap(Pool_, r.Pool_);
            DoSwap(Len_, r.Len_);
            DoSwap(Data_, r.Data_);
        }

        inline T& operator[] (size_t n) const noexcept {
            Y_ASSERT(n < Size());

            return *(Begin() + n);
        }

        inline T* Begin() const noexcept {
            return Data_;
        }

        inline T* End() const noexcept {
            return this->Begin() + this->Size();
        }

        inline size_t Size() const noexcept {
            return Len_;
        }

    private:
        TMsFixedVector(const TMsFixedVector&);
        TMsFixedVector& operator= (const TMsFixedVector&);

    private:
        TMemoryPool* Pool_;
        size_t Len_;
        T* Data_;
};
