#pragma once

#include "seq4.h"
#include "storage.h"
#include "types.h"

#include <kernel/text_machine/structured_id/full_id.h>
#include <kernel/text_machine/module/save_to_json.h>

#include <library/cpp/enumbitset/enumbitset.h>

#include <util/generic/hash.h>

namespace NTextMachine {
namespace NCore {
    namespace NValueInternals {
        struct TBlockIdTraits {
            using TValue = ui32;

            static Y_FORCE_INLINE TStringBuf GetName() {
                return TStringBuf("Block");
            }
        };
    } // NFloatInternals

    using TStreamsBitSet = TEnumBitSet<EStreamType, 0, ::NLingBoost::TStream::Size>;

    enum class EValuePartType {
        StreamSet = 0,
        StreamValue = 1,
        HitPrec = 2,
        FormsCount = 3
    };

    namespace NValueInternals {
        struct TValueIdBuilder : public ::NStructuredId::TIdBuilder<EValuePartType> {
            using TValuePartsList = TListType<
                TPair<EValuePartType::StreamSet, ::NStructuredId::TEnumBitSetMixin<TStreamsBitSet>>,
                TPair<EValuePartType::StreamValue, ::NStructuredId::TPartMixin<EHitWeightType>>,
                TPair<EValuePartType::HitPrec, ::NStructuredId::TPartMixin<EHitPrecType>>,
                TPair<EValuePartType::FormsCount, ::NStructuredId::TPartMixin<EFormsCountType>>
            >;
        };
    } // NFloatInternals
} // NCore
} // NTextMachine

namespace NStructuredId {
    template <>
    struct TGetIdTraitsList<NTextMachine::NCore::EValuePartType> {
        using TResult = NTextMachine::NCore::NValueInternals::TValueIdBuilder::TValuePartsList;
    };
} // NStructuredId

namespace NTextMachine {
namespace NCore {
    using TValueId = ::NStructuredId::TFullId<EValuePartType>;

    class TValueIdWithHash {
        TValueId Id;
        ui64 Hash = 0;
    public:
        TValueIdWithHash() = default;
        TValueIdWithHash(const TValueId& id)
            : Id(id), Hash(id.Hash())
        {}

        const TValueId& GetId() const {
            return Id;
        }
        ui64 GetHash() const {
            return Hash;
        }

        bool operator == (const TValueIdWithHash& other) const {
            return other.Hash == Hash && other.Id == Id;
        }
    };
} // NCore
} // NTextMachine

template<>
struct THash<NTextMachine::NCore::TValueIdWithHash> {
    ui64 operator() (const NTextMachine::NCore::TValueIdWithHash& id) const {
        return id.GetHash();
    }
};

namespace NTextMachine {
namespace NCore {
    template<typename T>
    using TValueRef = const T*;
    template<typename T>
    using TValueRefsHolder = TPoolPodHolder<TValueRef<T>>;
    template<typename T>
    using TValueRefsBuffer = TPodBuffer<TValueRef<T>>;

    using TFloatRef = TValueRef<float>;
    using TFloatRefsHolder = TValueRefsHolder<float>;
    using TFloatRefsBuffer = TValueRefsBuffer<float>;

    using TUint64Ref = TValueRef<ui64>;
    using TUint64RefsHolder = TValueRefsHolder<ui64>;
    using TUint64RefsBuffer = TValueRefsBuffer<ui64>;

    template<typename T>
    class TValuesRegistry {
    public:
        TValuesRegistry(TMemoryPool& pool, size_t numBuckets)
            : Pool(pool)
        {
            Data.Init(pool, numBuckets, EStorageMode::Full);
            Data.Construct();
        }
        TValueRefsHolder<T>& Alloc(size_t index, size_t count) {
            Data[index].Init(Pool, count, EStorageMode::Empty);
            return Data[index];
        }
        TValueRef<T> Get(i64 index, i64 valueIndex) const {
            return Data[index][valueIndex];
        }
        TValueRef<T> GetChecked(i64 index, i64 valueIndex) const {
            if (index >= 0
                && static_cast<size_t>(index) < Data.Count()
                && valueIndex >= 0
                && static_cast<size_t>(valueIndex) < Data[index].Count())
            {
                return Get(index, valueIndex);
            } else {
                Y_ASSERT(false);
                return GetZero();
            }
        }
        static TValueRef<T> GetZero() {
            static const T zero = 0.0;
            return &zero;
        }

    private:
        TMemoryPool& Pool;
        TPoolPodHolder<TValueRefsHolder<T>> Data;
    };

    using TFloatsRegistry = TValuesRegistry<float>;
    using TUints64Registry = TValuesRegistry<ui64>;

    template<typename T>
    class TValueRefCollection
        : public NModule::TJsonSerializable
    {
    private:
    public:
        void Init(TMemoryPool& pool, size_t n) {
            Values.Init(pool, n, EStorageMode::Full);
        }
        size_t Size() const {
            return Values.Count();
        }
        void Bind(size_t i, TValueRef<T> ref) {
            Y_ASSERT(ref);
            Values[i] = ref;
        }
        T Get(size_t i) const {
            Y_ASSERT(Values[i]);
            return *Values[i];
        }
        T operator[] (size_t i) const {
            Y_ASSERT(Values[i]);
            return *Values[i];
        }
        float CountNonZeroFraction() const {
            size_t result = 0;
            if (!Size()) {
                return 0.0f;
            }
            for (size_t i : xrange(Size())) {
                if (Get(i) > 0) {
                    result += 1;
                }
            }
            return float(result) / Size();
        }

        void BindToZero() {
            Values.Fill(TValuesRegistry<T>::GetZero());
        }

        void SaveToJson(NJson::TJsonValue& value) const {
            for (size_t i : xrange(Values.Count())) {
                value[i] = (nullptr == Values[i] ? NJson::TJsonValue("null") : NModule::JsonValue(*Values[i]));
            }
        }

    protected:
        TPoolPodHolder<TValueRef<T>> Values;
    };


    template<typename T>
    class TValueCollection
        : public NModule::TJsonSerializable
    {
    public:
        void Init(TMemoryPool& pool, size_t n) {
            Values.Init(pool, n, EStorageMode::Full);
        }
        size_t Size() const {
            return Values.Count();
        }
        T Get(size_t i) const {
            return Values[i];
        }
        T* Ptr(size_t i) const {
            return Values.Ptr(i);
        }
        T& operator[] (size_t i) const {
            return Values[i];
        }
        void MemSetZeroes() {
            Values.FillZeroes();
        }
        void Assign(T value) {
            Values.Fill(value);
        }
        float CountNonZeroFraction() const {
            size_t result = 0;
            if (!Size()) {
                return 0.0f;
            }
            for (size_t i : xrange(Size())) {
                if (Get(i) > 0) {
                    result += 1;
                }
            }
            return float(result) / Size();
        }
        void SaveToJson(NJson::TJsonValue& value) const {
            Values.SaveToJson(value);
        }

    protected:
        TPoolPodHolder<T> Values;
    };

    using TUint64Collection = TValueCollection<ui64>;
    using TUint64RefCollection = TValueRefCollection<ui64>;

    class TFloatCollection
        : public TValueCollection<float>
    {
    public:
        NSeq4f::TSeq4f AsSeq4f() const {
            return NSeq4f::TSeq4f(Values.Data(), Size());
        }
        template <typename SeqType>
        void CopyFrom(SeqType&& x) {
            Y_ASSERT(x.Avail() == Size());
            NSeq4f::Copy(std::forward<SeqType>(x), Values.Data());
        }
        template <typename SeqType>
        void AddFrom(SeqType&& x) {
            Y_ASSERT(x.Avail() == Size());
            NSeq4f::Copy(
                NSeq4f::Add(AsSeq4f(), std::forward<SeqType>(x)),
                Values.Data()
            );
        }
    };

    class TFloatRefCollection
        : public TValueRefCollection<float>
    {
    private:
        struct TFloatDerefOp {
            Y_FORCE_INLINE float operator() (TFloatRef ref) const {
                Y_ASSERT(ref);
                return *ref;
            }
        };
    public:
        auto AsSeq4f() const -> decltype(NSeq4f::Make((TFloatRef*) nullptr, 0, TFloatDerefOp())) {
            return NSeq4f::Make(Values.Data(), Size(), TFloatDerefOp());
        }
    };
} // NCore
} // NTextMachine
