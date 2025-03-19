#pragma once

#include "seq4.h"

#include <kernel/text_machine/module/save_to_json.h>

#include <util/generic/array_ref.h>
#include <util/generic/typetraits.h>

#include <array>
#include <type_traits>

namespace NTextMachine {
namespace NCore {
    namespace NDetail {
        template <typename KeyType, typename ValueType, KeyType... Keys>
        class TStaticTableTraits;

        template <typename KeyType, typename ValueType>
        class TStaticTableTraits<KeyType, ValueType> {
        public:
            using TKey = KeyType;
            using TValue = ValueType;
            using TValueParam = typename TTypeTraits<TValue>::TFuncParam;

            constexpr static size_t Size = 0;

            template <TKey Key>
            static bool HasKey(std::integral_constant<TKey, Key>) {
                return false;
            }
            static void GetIndex() {}
            static constexpr std::array<TKey, 0> GetKeys() {
                return {{}};
            }
        };

        template <typename KeyType, typename ValueType, KeyType Key, KeyType... Keys>
        class TStaticTableTraits<KeyType, ValueType, Key, Keys...>
            : public TStaticTableTraits<KeyType, ValueType, Keys...>
        {
        public:
            using TKey = KeyType;
            using TValue = ValueType;
            using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
            using TBase = TStaticTableTraits<KeyType, ValueType, Keys...>;

            constexpr static size_t Size = TBase::Size + 1;

            using TBase::GetIndex;

            static bool HasKey(std::integral_constant<TKey, Key>) {
                return true;
            }
            static size_t GetIndex(std::integral_constant<TKey, Key>) {
                return Size - 1;
            }
            static std::array<TKey, Size> GetKeys() {
                return {{Key, Keys...}};
            }
        };
    } // NDetail

    // Fixed size static vector with named components ("fields").
    // Useful in polymorphic code, i.e. same statement
    // can read / write different fields depending on instantiation context.
    // In case of float ValueType - easy SSE support.
    // For access, key value should be known at compile time.
    //
    template <typename KeyType, typename ValueType, KeyType... Keys>
    class TStaticTable
        : public NDetail::TStaticTableTraits<KeyType, ValueType, Keys...>
        , public NModule::TJsonSerializable
    {
    public:
        using TSelf = TStaticTable<KeyType, ValueType, Keys...>;
        using TTraits = NDetail::TStaticTableTraits<KeyType, ValueType, Keys...>;
        using TKey = typename TTraits::TKey;
        using TValue = typename TTraits::TValue;
        using TValueParam = typename TTraits::TValueParam;

        template <TKey Key>
        using TKeyGen = std::integral_constant<TKey, Key>;

        template <TKey Key>
        static TKeyGen<Key> MakeKey() {
            return {};
        }

        TStaticTable() = default;
        TStaticTable(TValueParam defValue) {
            Data.fill(defValue);
        }

        void Fill(TValueParam defValue) {
            Data.fill(defValue);
        }

        template <TKey Key>
        static bool Has()  {
            return TTraits::HasKey(TKeyGen<Key>{});
        }
        template <TKey Key>
        TValueParam Get() const {
            return Data[GetIndex<Key>()];
        }
        template <TKey Key>
        void Set(TValueParam value) {
            Data[GetIndex<Key>()] = value;
        }
        template <TKey Key>
        void SetOrIgnore(TValueParam value) {
            if (Has<Key>()) {
                Data[GetIndex<Key>()] = value;
            }
        }
        template <TKey Key>
        TValue& Ref() {
            return Data[GetIndex<Key>()];
        }

        template <TKey Key>
        Y_FORCE_INLINE TValueParam operator[] (TKeyGen<Key>) const {
            return Data[GetIndex<Key>()];
        }
        template <TKey Key>
        Y_FORCE_INLINE TValue& operator[] (TKeyGen<Key>) {
            return Data[GetIndex<Key>()];
        }

        const TValue* AsPtr() const {
            return &Data[0];
        }
        TValue* AsPtr() {
            return &Data[0];
        }

        void SaveToJson(NJson::TJsonValue& value) const {
            value = NJson::TJsonValue(NJson::JSON_MAP);
            int index = static_cast<int>(TTraits::Size) - 1;
            for (auto key : TTraits::GetKeys()) {
                Y_ASSERT(index >= 0);
                value[ToString(key)] = NModule::JsonValue(Data[index]);
                index -= 1;
            }
        }

        // float-only methods
        //

        NSeq4f::TSeq4f AsSeq4f() const {
            return NSeq4f::TSeq4f(&Data[0], Data.size());
        }

        template <typename SeqType>
        void AddFrom(SeqType&& x) {
            Y_ASSERT(x.Avail() == TTraits::Size);
            NSeq4f::Copy(
                NSeq4f::Add(AsSeq4f(), std::forward<SeqType>(x)),
                &Data[0]
            );
        }

        void AddFrom(const TSelf& x) {
            AddFrom(x.AsSeq4f());
        }

        template <typename SeqType>
        void MulFrom(SeqType&& x) {
            Y_ASSERT(x.Avail() == TTraits::Size);
            NSeq4f::Copy(
                NSeq4f::Mul(AsSeq4f(), std::forward<SeqType>(x)),
                &Data[0]
            );
        }

        void MulFrom(const TSelf& x) {
            MulFrom(x.AsSeq4f());
        }

        template <typename SeqType>
        void CopyFrom(SeqType&& x) {
            Y_ASSERT(x.Avail() == TTraits::Size);
            NSeq4f::Copy(std::forward<SeqType>(x), &Data[0]);
        }

        //
        // end of float-only methods

    private:
        template <TKey Key>
        Y_FORCE_INLINE static size_t GetIndex() {
            return TTraits::GetIndex(TKeyGen<Key>{});
        }

    private:
        std::array<TValue, TTraits::Size> Data;
    };
} // NCore
} // NTextMachine
