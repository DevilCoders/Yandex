#pragma once

#include <library/cpp/enumbitset/enumbitset.h>

#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/str_stl.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/digest/multi.h>

namespace NStructuredId {
    template <typename EIdPartType>
    using TStringId = TMap<EIdPartType, TString>;

    template <typename T>
    class TPartMixin {
    public:
        using TValue = T;
        using TValueParam = typename TTypeTraits<TValue>::TFuncParam;

    protected:
        TPartMixin() = default;
        TPartMixin(const TPartMixin<TValue>&) = default;
        TPartMixin<TValue>& operator = (const TPartMixin<TValue>&) = default;

        Y_FORCE_INLINE bool operator == (const TPartMixin<TValue>& rhs) const {
            return Valid == rhs.Valid && PartValue == rhs.PartValue;
        }
        Y_FORCE_INLINE bool operator < (const TPartMixin<TValue>& rhs) const {
            return (!Valid && rhs.Valid) || (Valid && rhs.Valid && PartValue < rhs.PartValue);
        }
        Y_FORCE_INLINE TString ToString() const {
            return Valid ? ::ToString<TValue>(PartValue) : TString();
        }
        Y_FORCE_INLINE void FromString(TStringBuf text) {
            PartValue = ::FromString<TValue>(text);
            Valid = true;
        }
        Y_FORCE_INLINE TString Name() const {
            return ToString();
        }
        Y_FORCE_INLINE bool IsValid() const {
            return Valid;
        }
        Y_FORCE_INLINE void Set(TValueParam id) {
            PartValue = id;
            Valid = true;
        }
        Y_FORCE_INLINE TValue Get() const {
            return PartValue;
        }
        Y_FORCE_INLINE void UnSet() {
            Valid = false;
        }

    public:
        ui64 Hash() const {
            return Valid ? MultiHash(PartValue) : MultiHash(0);
        }

    private:
        bool Valid = false;
        TValue PartValue = TValue(); // Used to be just "Value", but MSVC goes crazy
                                     // when compiling THashSet<TId> due to imaginary
                                     // name conflict with THashSet template arg
    };

    template <typename T>
    class TEnumBitSetMixin;

    template <typename TEnum, int mbegin, int mend>
    class TEnumBitSetMixin<TEnumBitSet<TEnum, mbegin, mend>> {
    public:
        using TBitSet = TEnumBitSet<TEnum, mbegin, mend>;
        using TThis = TEnumBitSetMixin<TBitSet>;
        using TValue = TBitSet;
        // NOTE. Here TEnum is used intentionally.
        // It enables simplified syntax id.Set(enumValue) for
        // bit-set parts.
        using TValueParam = typename TTypeTraits<TEnum>::TFuncParam;

    protected:
        TEnumBitSetMixin() = default;
        TEnumBitSetMixin(const TThis&) = default;
        TThis& operator = (const TThis&) = default;

        Y_FORCE_INLINE bool operator == (const TThis& rhs) const {
            return PartValue == rhs.PartValue;
        }
        Y_FORCE_INLINE bool operator < (const TThis& rhs) const {
            return PartValue < rhs.PartValue;
        }
        Y_FORCE_INLINE TString ToString(TStringBuf sep = "|") const {
            TString res;
            for (const auto& elem : PartValue) {
                if (!res.empty()) {
                    res.append(sep);
                }
                const TString elemStr = ::ToString(elem);
                res += elemStr;
            }
            return res;
        }
        Y_FORCE_INLINE void FromString(TStringBuf text, TStringBuf sep = "|") {
            Y_ASSERT(!sep.empty());
            for (auto& it : StringSplitter(text).SplitByString(sep)) {
                TStringBuf elemStr = it.Token();
                PartValue.Set(::FromString<TEnum>(elemStr));
            }
        }
        Y_FORCE_INLINE TString Name() const {
            return ToString("");
        }
        Y_FORCE_INLINE bool IsValid() const {
            return !PartValue.Empty();
        }
        Y_FORCE_INLINE void Set(TValueParam id) {
            PartValue.Set(id);
        }
        Y_FORCE_INLINE TValue Get() const {
            return PartValue;
        }
        Y_FORCE_INLINE void UnSet() {
            PartValue.Reset();
        }
        Y_FORCE_INLINE void ToStringVector(TVector<TString>& out) const {
            for (const auto& elem : PartValue) {
                out.push_back(ToString(elem));
            }
        }

    public:
        ui64 Hash() const {
            return PartValue.GetHash();
        }

    private:
        TValue PartValue{}; // See comment in TPartMixin
    };

    namespace NDetail {
        class TNullId {
        public:
            TNullId() = default;
            TNullId(const TNullId&) = default;
            TNullId& operator = (const TNullId&) = default;

            Y_FORCE_INLINE bool operator == (const TNullId&) const {
                return true;
            }
            Y_FORCE_INLINE bool operator < (const TNullId&) const {
                return false;
            }
            template <typename EIdPartType>
            Y_FORCE_INLINE void MakeStringId(TStringId<EIdPartType>&) const {
            }
            template <typename EIdPartType>
            Y_FORCE_INLINE void ParseStringId(const TStringId<EIdPartType>&) {
            }
            Y_FORCE_INLINE TString FullName(const TStringBuf&) const {
                return "";
            }
            template <typename FuncType>
            static void ForEach(FuncType&) {
            }
        };

        template <typename EIdPartType, EIdPartType Part>
        struct TIdTraits;

        template <typename EIdPartType, EIdPartType Part>
        struct TSelectPart {};

        template <typename EIdPartType, EIdPartType Part>
        class TIdImpl
            : public NStructuredId::NDetail::TIdTraits<EIdPartType, Part>::TBase
            , public NStructuredId::NDetail::TIdTraits<EIdPartType, Part>::TMixin
        {
        public:
            using TThis = TIdImpl<EIdPartType, Part>;
            template <EIdPartType PartY>
            using TOther = TIdImpl<EIdPartType, PartY>;

            using TTraits = NStructuredId::NDetail::TIdTraits<EIdPartType, Part>;
            template <EIdPartType PartY>
            using TTraitsY = NStructuredId::NDetail::TIdTraits<EIdPartType, PartY>;

            template <EIdPartType PartY>
            using TSelectPart = NStructuredId::NDetail::TSelectPart<EIdPartType, PartY>;

            using TBase = typename TTraits::TBase;
            using TMixin = typename TTraits::TMixin;
            using TValue = typename TMixin::TValue;
            using TValueParam = typename TMixin::TValueParam;

            template <EIdPartType PartY>
            using TBaseY = typename TTraitsY<PartY>::TBase;
            template <EIdPartType PartY>
            using TMixinY = typename TTraitsY<PartY>::TMixin;
            template <EIdPartType PartY>
            using TValueY = typename TMixinY<PartY>::TValue;
            template <EIdPartType PartY>
            using TValueParamY = typename TMixinY<PartY>::TValueParam;

        public:
            TIdImpl() = default;
            TIdImpl(const TThis&) = default;
            TThis& operator = (const TThis&) = default;

            Y_FORCE_INLINE bool operator == (const TThis& rhs) const {
                return TMixin::operator==(rhs) && TBase::operator==(rhs);
            }
            Y_FORCE_INLINE bool operator < (const TThis& rhs) const {
                return TMixin::operator<(rhs) || (TMixin::operator==(rhs) && TBase::operator<(rhs));
            }
            Y_FORCE_INLINE ui64 Hash() const {
                return MultiHash(static_cast<const TMixin&>(*this), static_cast<const TBase&>(*this));
            }
            Y_FORCE_INLINE TString FullName(const TStringBuf& sep = TStringBuf("_")) const {
                TString childName = TBase::FullName(sep);
                TString thisName = TMixin::Name();
                return childName && thisName ? thisName + sep + childName : thisName + childName;
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE TString SuffixName(const TStringBuf& sep = TStringBuf("_")) const {
                return SuffixNameImpl(TSelectPart<PartY>(), sep);
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE bool IsValid() const {
                return IsValidImpl(TSelectPart<PartY>());
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE TValueY<PartY> Get() const {
                return GetImpl(TSelectPart<PartY>());
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE void UnSet() {
                return UnSetImpl(TSelectPart<PartY>());
            }
            Y_FORCE_INLINE void Set(TValueParam value) {
                TMixin::Set(value);
            }
            template <typename ValueY>
            Y_FORCE_INLINE void Set(ValueY value) {
                TBase::Set(value);
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE void Set(const TOther<PartY>& rhs) {
                CopyImpl(rhs);
            }

            template <typename FuncType>
            Y_FORCE_INLINE static void ForEach(FuncType& func) {
                func.template Do<Part>();
                TBase::ForEach(func);
            }

        protected:
            Y_FORCE_INLINE void MakeStringId(TStringId<EIdPartType>& strId) const {
                if (TMixin::IsValid()) {
                    strId[Part] = TMixin::ToString();
                }
                TBase::MakeStringId(strId);
            }
            Y_FORCE_INLINE void ParseStringId(const TStringId<EIdPartType>& strId) {
                auto iter = strId.find(Part);
                if (iter != strId.end()) {
                    TMixin::FromString(iter->second);
                }
                TBase::ParseStringId(strId);
            }

        public:
            static TThis FromStringId(const TStringId<EIdPartType>& strId) {
                TThis id;
                id.ParseStringId(strId);
                return id;
            }

            static TStringId<EIdPartType> ToStringId(const TThis& id) {
                TStringId<EIdPartType> strId;
                id.MakeStringId(strId);
                return strId;
            }

        protected:
            template <EIdPartType PartY>
            Y_FORCE_INLINE void CopyImpl(const TOther<PartY>& rhs)
            {
                static_cast<TOther<PartY>&>(*this) = rhs;
            }
            Y_FORCE_INLINE void CopyImpl(const TThis& rhs)
            {
                *this = rhs;
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE TString SuffixNameImpl(TSelectPart<PartY> x, const TStringBuf& sep) const {
                return TBase::SuffixNameImpl(x, sep);
            }
            Y_FORCE_INLINE TString SuffixNameImpl(TSelectPart<Part>, const TStringBuf& sep) const {
                return FullName(sep);
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE bool IsValidImpl(TSelectPart<PartY> x) const {
                return TBase::IsValidImpl(x);
            }
            Y_FORCE_INLINE bool IsValidImpl(TSelectPart<Part>) const {
                return TMixin::IsValid();
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE TValueY<PartY> GetImpl(TSelectPart<PartY> x) const {
                return TBase::GetImpl(x);
            }
            Y_FORCE_INLINE TValue GetImpl(TSelectPart<Part>) const {
                return TMixin::Get();
            }
            template <EIdPartType PartY>
            Y_FORCE_INLINE void UnSetImpl(TSelectPart<PartY> x) {
                return TBase::UnSetImpl(x);
            }
            Y_FORCE_INLINE void UnSetImpl(TSelectPart<Part>) {
                return TMixin::UnSet();
            }
        };
    } // NDetail
} // NStructuredId

template <>
struct THash<::NStructuredId::NDetail::TNullId> {
    ui64 operator() (const ::NStructuredId::NDetail::TNullId&) {
        return MultiHash(0);
    }
};

template <typename T>
struct THash<::NStructuredId::TPartMixin<T>> {
    ui64 operator() (const ::NStructuredId::TPartMixin<T>& x) {
        return x.Hash();
    }
};

template <typename T>
struct THash<::NStructuredId::TEnumBitSetMixin<T>> {
    ui64 operator() (const ::NStructuredId::TEnumBitSetMixin<T>& x) {
        return x.Hash();
    }
};

template <typename EIdPartType, EIdPartType Part>
struct THash<::NStructuredId::NDetail::TIdImpl<EIdPartType, Part>> {
    ui64 operator() (const ::NStructuredId::NDetail::TIdImpl<EIdPartType, Part>& x) {
        return x.Hash();
    }
};
