#pragma once

#include "enum_map.h"

#include <util/string/cast.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>
#include <util/generic/singleton.h>

namespace NLingBoost {
    template <typename EnumStruct>
    class TEnumOps
        : public EnumStruct
    {
    public:
        using TEnumStruct = EnumStruct;
        using TEnum = typename EnumStruct::EType;

        static const TVector<TEnum>& GetValuesVector() {
            static const TVector<TEnum> values(EnumStruct::GetValues().begin(), EnumStruct::GetValues().end());
            return values;
        }
        static const TSet<TEnum>& GetValuesSet() {
            static const TSet<TEnum> valuesSet(EnumStruct::GetValues().begin(), EnumStruct::GetValues().end());
            return valuesSet;
        }
        static TArrayRef<const TEnum> GetValuesRegion() {
            return EnumStruct::GetValues();
        }
        static const TCompactEnumRemap<TEnumStruct>& GetFullRemap() {
            static const TCompactEnumRemap<TEnumStruct> remap(GetValuesRegion());
            return remap;
        }

        template <typename ContType>
        static void SaveValues(ContType& cont) {
            cont.insert(EnumStruct::GetValues().begin(), EnumStruct::GetValues().end());
        }

        static bool HasValue(TEnum value) {
            return GetValuesSet().contains(value);
        }
        static bool HasIndex(i32 index) {
            const auto& valuesRegion = GetValuesRegion();
            return valuesRegion.size() > 0
                && static_cast<i32>(valuesRegion[0]) <= index
                && static_cast<i32>(valuesRegion.back()) >= index
                && HasValue(static_cast<TEnum>(index));
        }
    };

    class IIntegralType {
    public:
        virtual ~IIntegralType() {}

        virtual bool HasValueIndex(i32 index) const = 0;
        virtual TStringBuf GetTypeCppName() const = 0;
        virtual TStringBuf GetValueScopePrefix() const = 0;
        virtual TString GetValueLiteral(i32 index) const = 0;
    };

    template <typename EnumStruct>
    class TEnumTypeDescr
        : public IIntegralType
    {
    private:
        using TOps = TEnumOps<EnumStruct>;
        using TEnum = typename EnumStruct::EType;
        TString ScopePrefix;

    public:
        ~TEnumTypeDescr() {}

        TEnumTypeDescr() {
            ScopePrefix.append(TOps::GetFullName());
            ScopePrefix.append(TStringBuf("::"));
        }

        bool HasValueIndex(i32 index) const final {
            return TOps::HasIndex(index);
        }
        TStringBuf GetTypeCppName() const final {
            return TOps::GetFullName();
        }
        TStringBuf GetValueScopePrefix() const final {
            return ScopePrefix;
        }
        TString GetValueLiteral(i32 index) const final {
            return TOps::HasIndex(index)
                ? ToString(static_cast<TEnum>(index))
                : TString{};
        }

        static const IIntegralType& Instance() {
            return *Singleton<TEnumTypeDescr<EnumStruct>>();
        }
    };

    namespace NDetail {
        template <typename ValueType, ValueType... Values>
        constexpr bool IsStrictlyOrdered() {
            const ValueType values[] = {Values...};
            for (size_t i = 0; i + 1 < sizeof...(Values); ++i) {
                if (values[i + 1] <= values[i]) {
                    return false;
                }
            }
            return true;
        }

        template <typename ValueType, ValueType... Values>
        inline TArrayRef<const ValueType> GetStaticRegion() {
            if constexpr (sizeof...(Values) == 0) {
                return {};
            } else {
                static_assert(IsStrictlyOrdered<ValueType, Values...>(), "values should be unique and ordered");
                static const ValueType values[] = {Values...};
                return {values, sizeof...(Values)};
            }
        }
    } // NDetail
} // NLingBoost
