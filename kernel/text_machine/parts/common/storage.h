#pragma once

#include "seq4.h"

#include <kernel/text_machine/module/module.h>
#include <kernel/text_machine/module/save_to_json.h>

#include <util/random/mersenne.h>
#include <util/memory/pool.h>
#include <util/generic/algorithm.h>
#include <util/generic/typetraits.h>
#include <util/generic/xrange.h>

namespace NTextMachine {
namespace NCore {
    using NLingBoost::TStaticEnumMap;
    using NLingBoost::TPoolableEnumMap;
    using NLingBoost::TPoolableCompactEnumMap;
}
}

namespace NTextMachine {
namespace NCore {
    template <typename T, typename... Args>
    Y_FORCE_INLINE void Construct(T& obj, Args&&... args) {
        new (&obj) T(std::forward<Args>(args)...);
    }

    class TTMTestEnviron;

    template <typename T>
    class TPoolPodHolder;

    enum class EStorageMode {
        Empty,
        Full,
    };

    enum class EInitializeMode {
        None,
        Zeroes,
        Ones,
        Random
    };

    template <typename T>
    class TPodBuffer
        : public NModule::TJsonSerializable
    {
    protected:
        T* BufData = nullptr;
        size_t BufSize = 0;
        size_t BufIndex = 0;

        template <typename X> friend class TPodBuffer;

    public:
        static TPodBuffer<T> FromVector(TVector<std::remove_const_t<T>>& x,
            EStorageMode mode = EStorageMode::Full)
        {
            return TPodBuffer<T>(x.data(), x.size(), mode);
        }
        static TPodBuffer<T> FromVector(const TVector<std::remove_const_t<T>>& x,
            EStorageMode mode = EStorageMode::Full)
        {
            return TPodBuffer<T>(x.data(), x.size(), mode);
        }
        static TPodBuffer<T> FromVector(const TVector<const std::remove_const_t<T>>& x,
            EStorageMode mode = EStorageMode::Full)
        {
            return TPodBuffer<T>(~x, +x, mode);
        }

    public:
        TPodBuffer() = default;
        template <typename X>
        TPodBuffer(const TPodBuffer<X>& other)
            : BufData(other.BufData)
            , BufSize(other.BufSize)
            , BufIndex(other.BufIndex)
        {}
        TPodBuffer(T* data, size_t size, EStorageMode mode = EStorageMode::Empty)
            : BufData(data)
            , BufSize(size)
            , BufIndex(EStorageMode::Empty == mode ? 0 : size)
        {
        }
        void Init(T* data, size_t size, EStorageMode mode = EStorageMode::Empty) {
            *this = TPodBuffer<T>(data, size, mode);
        }
        void Construct() {
            new (BufData) T[BufSize];
        }
        void Destruct() {
            for (size_t i : xrange(BufSize)) {
                BufData[i].~T();
            }
        }

        size_t Avail() const {
            return BufSize - BufIndex;
        }
        size_t Capacity() const {
            return BufSize;
        }
        size_t Count() const {
            return BufIndex;
        }
        size_t size() const {
            return BufIndex;
        }
        T* Data() const {
            return BufData;
        }
        T* Begin() const {
            return BufData;
        }
        T* End() const {
            return BufData + BufIndex;
        }
        T* begin() const {
            return BufData;
        }
        T* end() const {
            return BufData + BufIndex;
        }
        T* Ptr(size_t i) const {
            Y_ASSERT(i < BufSize);
            return BufData + i;
        }
        T& Ref(size_t i) const {
            Y_ASSERT(i < BufSize);
            return BufData[i];
        }
        T& operator[](size_t i) const {
            Y_ASSERT(i < BufSize);
            return BufData[i];
        }
        T& Front() const {
            return *BufData;
        }
        T& Back() const {
            Y_ASSERT(BufIndex > 0);
            return *((BufData + BufIndex) - 1);
        }
        T& Cur() const {
            Y_ASSERT(BufIndex < BufSize);
            return BufData[BufIndex];
        }
        TPodBuffer<T> Sub(size_t beginIndex, size_t endIndex) {
            Y_ASSERT(beginIndex <= endIndex);
            Y_ASSERT(endIndex <= BufSize);
            return TPodBuffer<T>(BufData + beginIndex, endIndex - beginIndex, EStorageMode::Full);
        }

        template <typename... Args>
        void Emplace(Args&&... args) {
            Y_ASSERT(BufIndex < BufSize);
            new (&BufData[BufIndex++]) T(std::forward<Args>(args)...);
        }
        void Add(typename TTypeTraits<T>::TFuncParam value) {
            Emplace(value);
        }
        TPodBuffer<T> Append(size_t n, EStorageMode mode = EStorageMode::Empty) {
            Y_ASSERT(BufIndex + n <= BufSize);
            size_t index = BufIndex;
            BufIndex += n;
            return TPodBuffer<T>(BufData + index, n, mode);
        }
        TPodBuffer<T> Remove() {
            Y_ASSERT(BufIndex > 0);
            BufIndex -= 1;
            return TPodBuffer<T>(BufData + BufIndex, 1, EStorageMode::Full);
        }
        TPodBuffer<T> Clear() {
            size_t index = BufIndex;
            BufIndex = 0;
            return TPodBuffer<T>(BufData, index, EStorageMode::Full);
        }
        void SetFull() {
            BufIndex = BufSize;
        }
        void SetTo(size_t size) {
            Y_ASSERT(size <= BufSize);
            BufIndex = size;
        }
        void FillZeroes() {
            memset(BufData, 0, BufSize * sizeof(T));
        }
        void Fill(typename TTypeTraits<T>::TFuncParam value) {
            ::Fill(BufData, BufData + BufSize, value);
        }

        NSeq4f::TSeq4f AsSeq4f() const {
            return NSeq4f::TSeq4f(BufData, BufSize);
        }
        NSeq4f::TSeq<T> AsSeq() const {
            return NSeq4f::TSeq<T>(BufData, BufSize);
        }
        void SaveToJson(NJson::TJsonValue& value) const {
            for (size_t i : xrange(BufIndex)) {
                value[i] = NModule::JsonValue(BufData[i]);
            }
        }
    };

    class TStorageStaticParams {
        static EInitializeMode InitMode; // For test purposes only.
                                         // Do not change in production code.
        friend class TTMTestEnviron;
        template <typename T>
        friend class TPoolPodHolder;
    };

    template <typename T>
    class TPoolPodHolder
        : public TPodBuffer<T>
    {
    public:
        TPoolPodHolder() = default;
        TPoolPodHolder(TMemoryPool& pool, size_t size, EStorageMode mode = EStorageMode::Empty)
            : TPodBuffer<T>(pool.AllocateArray<T>(size), size, mode)
        {
            void* data = static_cast<void*>(TPodBuffer<T>::Data());

            if (EInitializeMode::Zeroes == TStorageStaticParams::InitMode) {
                memset(data, 0, size * sizeof(T));
            }
            if (EInitializeMode::Ones == TStorageStaticParams::InitMode) {
                memset(data, 0xFF, size * sizeof(T));
            }
            if (EInitializeMode::Random == TStorageStaticParams::InitMode) {
                ui8* bytes = static_cast<ui8*>(data);
                TMersenne<size_t> rand(size);
                for (size_t i : xrange(size * sizeof(T))) {
                    bytes[i] = (ui8)rand.Uniform(0xFF);
                }
            }
        }
        void Init(TMemoryPool& pool, size_t size, EStorageMode mode = EStorageMode::Empty) {
            *this = TPoolPodHolder<T>(pool, size, mode);
        }
    };

} // NCore
} // NTextMachine

bool FromString(const TStringBuf&, NTextMachine::NCore::EInitializeMode&);
