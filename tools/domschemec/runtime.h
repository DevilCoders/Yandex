#pragma once

#if !defined(runtime_h_included_siaudtusdyf)
#define runtime_h_included_siaudtusdyf

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <array>
#include <functional>
#include <initializer_list>
#include <type_traits>

class IOutputStream;

namespace NDomSchemeRuntime {
    struct TValidateInfo {
        enum class ESeverity : ui8 {
            Error,
            Warning
        };
        ESeverity Severity;

        TValidateInfo()
            : Severity(ESeverity::Error)
        {
        }
        TValidateInfo(ESeverity severity)
            : Severity(severity)
        {
        }
    };
    using TErrorCallback = std::function<void(TString, TString)>;
    using TErrorCallbackEx = std::function<void(TString, TString, TValidateInfo)>;

    template <typename T>
    struct TPrimitiveTypeTraits {
        static TStringBuf TypeName() {
            return "unknown type"sv;
        }
        // for non-primive types
        template<typename TTraits> using TWrappedType = T;
        template<typename TTraits> using TConstWrappedType = T;
    };

    template <typename TValueRef, typename TTraits>
    struct TValueCommon {
    private:
        TValueCommon& operator=(const TValueCommon&) = delete;
    public:
        TValueCommon(TValueRef value)
            : Value__(value)
        {
        }

        TValueCommon(const TValueCommon&) = default;

        TValueRef Value__;

        bool IsNull() const {
            return TTraits::IsNull(Value__);
        }

        TValueRef GetRawValue() const {
            return Value__;
        }

        TString ToJson() const {
            return TTraits::ToJson(Value__);
        }
    };

    template<typename TTraits, typename TValueRef, typename T>
    struct TTraitsWrapper {
        static bool IsValidPrimitive(const T& t, TValueRef v) {
            return TTraits::IsValidPrimitive(t, v);
        }

        static void Get(TValueRef value, const T& def, T& ret) {
            TTraits::Get(value, def, ret);
        }

        static void Get(TValueRef value, T& ret) {
            TTraits::Get(value, ret);
        }

        static void Set(TValueRef value, const T& t) {
            TTraits::Set(value, t);
        }
    };

    template<typename TTraits, typename TValueRef>
    struct TTraitsWrapper<TTraits, TValueRef, TDuration> {
        static bool IsValidPrimitive(const TDuration&, TValueRef v) {
            typename TTraits::TStringType str;
            if (!TTraits::IsNull(v) && TTraits::IsValidPrimitive(str, v)) {
                TTraits::Get(v, str);
                TDuration d;
                return TDuration::TryParse(str, d);
            }
            double doubleValue;
            return TTraits::Get(v, 0., doubleValue);
        }

        static void Get(TValueRef value, const TDuration& def, TDuration& ret) {
            typename TTraits::TStringType stringValue;
            if (!TTraits::IsNull(value) && TTraits::IsValidPrimitive(stringValue, value)) {
                TTraits::Get(value, stringValue);
                ret = TDuration::Parse(stringValue);
                return;
            }
            double doubleValue;
            // we pass default value but that doesn't matter: we don't use it if function returns false
            if (!TTraits::IsNull(value) && TTraits::Get(value, 0., doubleValue)) {
                ret = TDuration::Parse(ToString(doubleValue));
                return;
            }
            ret = def;
        }

        static void Get(TValueRef value, TDuration& ret) {
            typename TTraits::TStringType stringValue;
            if (TTraits::IsValidPrimitive(stringValue, value)) {
                TTraits::Get(value, stringValue);
                ret = TDuration::Parse(stringValue);
                return;
            }
            double doubleValue;
            TTraits::Get(value, doubleValue);
            ret = TDuration::Parse(ToString(doubleValue));
        }

        static void Set(TValueRef value, const TDuration& t) {
            TTraits::Set(value, t.ToString());
        }
    };

    template<typename T>
    inline bool TryFromStringWrapper(const TStringBuf& src, T& dst) {
        return ::TryFromString<T>(src, dst);
    }

    template<>
    inline bool TryFromStringWrapper<TStringBuf>(const TStringBuf& src, TStringBuf& dst) {
        dst = src;
        return true;
    }

    template <typename TValueRef, typename TTraits, typename T>
    struct TConstPrimitiveImpl : public TValueCommon<TValueRef, TTraits> {
        using TBase = TValueCommon<TValueRef, TTraits>;
        bool WithDefault;
        T Default;

        explicit TConstPrimitiveImpl(TValueRef value)
            : TBase(value)
            , WithDefault(false)
        {
        }

        explicit TConstPrimitiveImpl(TValueRef value, T def)
            : TBase(value)
            , WithDefault(true)
            , Default(def)
        {
        }

        inline T Get() const {
            T ret;
            if (WithDefault) {
                TTraitsWrapper<TTraits, TValueRef, T>::Get(this->Value__, Default, ret);
            } else {
                TTraitsWrapper<TTraits, TValueRef, T>::Get(this->Value__, ret);
            }
            return ret;
        }

        template <class U>
        inline T Get(U&& def) const {
            T ret;
            TTraitsWrapper<TTraits, TValueRef, T>::Get(this->Value__, std::forward<U>(def), ret);
            return ret;
        }

        struct TRef {
            T Value;
            T* operator->() {
                return &Value;
            }
        };

        inline TRef operator->() const {
            return TRef { this->Get() };
        }

        inline T operator*() const {
            return this->Get();
        }

        inline operator T() const {
            return this->Get();
        }

        // need to implement explicitly because of this bug: https://clubs.at.yandex-team.ru/stackoverflow/2368
        template<typename T2>
        bool operator==(const T2& rhs) const {
            return this->Get() == rhs;
        }

        bool operator==(const TConstPrimitiveImpl& rhs) const {
            return this->Get() == rhs.Get();
        }

        #ifndef __cpp_impl_three_way_comparison
        template<typename T2>
        bool operator!=(const T2& rhs) const {
            return this->Get() != rhs;
        }
        #endif

        template <typename THelper = std::nullptr_t>
        bool Validate(const TString& path, bool /*strict*/, const TErrorCallbackEx& onError, THelper /*helper*/ = THelper()) const {
            T tmp{};
            bool ok = true;
            if (!TTraits::IsNull(this->Value__)) {
                if (!TTraitsWrapper<TTraits, TValueRef, T>::IsValidPrimitive(tmp, this->Value__)) {
                    if (onError) {
                        onError(path, TString("is not ") + TPrimitiveTypeTraits<T>::TypeName(), TValidateInfo());
                    }
                    ok = false;
                }
            }
            return ok;
        }

        bool Validate(const TString& path, bool strict, const TErrorCallback& onError) const {
            return Validate(path, strict, [&onError](TString p, TString e, TValidateInfo) { if (onError) { onError(p, e); } });
        }
    };

    template <typename TTraits, typename T>
    struct TConstPrimitive : public TConstPrimitiveImpl<typename TTraits::TConstValueRef, TTraits, T> {
        using TBase = TConstPrimitiveImpl<typename TTraits::TConstValueRef, TTraits, T>;
        using TConstValueRef = const typename TTraits::TConstValueRef;
        using TConst = TConstPrimitive<TTraits, T>;

        explicit TConstPrimitive(TConstValueRef value)
            : TBase(value)
        {
        }

        explicit TConstPrimitive(TConstValueRef value, T def)
            : TBase(value, def)
        {
        }

        friend inline IOutputStream& operator<< (IOutputStream& out, const TConstPrimitive& v) {
            return (out << v.Get());
        }
    };


    template <typename TTraits, typename T>
    struct TPrimitive : public TConstPrimitiveImpl<typename TTraits::TValueRef, TTraits, T> {
        using TBase = TConstPrimitiveImpl<typename TTraits::TValueRef, TTraits, T>;
        using TValueRef = typename TTraits::TValueRef;
        using TConst = TConstPrimitive<TTraits, T>;

        explicit TPrimitive(TValueRef value)
            : TBase(value)
        {
        }

        explicit TPrimitive(TValueRef value, T def)
            : TBase(value, def)
        {
        }

        TPrimitive(const TPrimitive& t) = default;

        inline void Set(const T& t) {
            TTraitsWrapper<TTraits, TValueRef, T>::Set(this->Value__, t);
        }

        using TBase::operator==;

        template <typename T2>
        inline TPrimitive& operator=(const T2& t) {
            this->Set(t);
            return *this;
        }

        inline TPrimitive& operator=(const TPrimitive& t) {
            this->Set(t);
            return *this;
        }

        inline TPrimitive& operator+=(const T& t) {
            this->Set(this->Get() + t);
            return *this;
        }

        friend inline IOutputStream& operator<< (IOutputStream& out, const TPrimitive& v) {
            return (out << v.Get());
        }
    };

#define REGISTER_PRIMITIVE_TYPE(type, name)     \
    template <>                                 \
    struct TPrimitiveTypeTraits<type> {         \
        static constexpr TStringBuf TypeName() { \
            return name;                        \
        }                                       \
        template <typename TTraits> using TWrappedType = TPrimitive<TTraits, type>; \
        template <typename TTraits> using TConstWrappedType = TConstPrimitive<TTraits, type>; \
    };

    REGISTER_PRIMITIVE_TYPE(bool, "bool")
    REGISTER_PRIMITIVE_TYPE(i8, "i8")
    REGISTER_PRIMITIVE_TYPE(i16, "i16")
    REGISTER_PRIMITIVE_TYPE(i32, "i32")
    REGISTER_PRIMITIVE_TYPE(i64, "i64")
    REGISTER_PRIMITIVE_TYPE(ui8, "ui8")
    REGISTER_PRIMITIVE_TYPE(ui16, "ui16")
    REGISTER_PRIMITIVE_TYPE(ui32, "ui32")
    REGISTER_PRIMITIVE_TYPE(ui64, "ui64")
    REGISTER_PRIMITIVE_TYPE(double, "double")
    REGISTER_PRIMITIVE_TYPE(TStringBuf, "string")
    REGISTER_PRIMITIVE_TYPE(TString, "string")
    REGISTER_PRIMITIVE_TYPE(TDuration, "duration")

#undef REGISTER_PRIMITIVE_TYPE

    template <typename TTraits, typename TEnable = void>
    struct TDefaultValue {
        using TConstValueRef = typename TTraits::TConstValueRef;

        static TConstValueRef Or(TConstValueRef value) {
            return value;
        }
    };

    Y_HAS_SUBTYPE(TValue, ValueType);

    template <typename TTraits>
    struct TDefaultValue<TTraits, std::enable_if_t<THasValueType<TTraits>::value>> {
        typename TTraits::TValue Value;

        TDefaultValue() = default;

        template <typename T>
        TDefaultValue(T&& def):
            Value(TTraits::Value(std::forward<T>(def)))
        {
        }

        using TConstValueRef = typename TTraits::TConstValueRef;

        TConstValueRef Or(TConstValueRef value) const {
            return TTraits::IsNull(value) ? TTraits::Ref(Value) : value;
        }
    };

    template <typename TValueRef, typename TTraits, typename T>
    struct TConstArrayImpl : public TValueCommon<TValueRef, TTraits> {
        using TBase = TValueCommon<TValueRef, TTraits>;
        bool WithDefault;
        TDefaultValue<TTraits> Default;

        explicit TConstArrayImpl(TValueRef value)
            : TBase(value)
            , WithDefault(false)
        {
        }

        template <typename U>
        TConstArrayImpl(TValueRef value, std::initializer_list<U> def)
            : TBase(value)
            , WithDefault(true)
            , Default(def)
        {
        }

        typename TTraits::TConstValueRef Get() const {
            if (WithDefault) {
                return Default.Or(this->Value__);
            } else {
                return this->Value__;
            }
        }

        struct TConstIterator {
            using iterator_category = std::forward_iterator_tag;
            using value_type = typename T::TConst;
            using difference_type = ptrdiff_t;
            using pointer = typename T::TConst*;
            using reference = typename T::TConst&;

            const TConstArrayImpl* A;
            typename TTraits::TArrayIterator It;

            inline typename T::TConst operator*() const {
                return typename T::TConst { TTraits::ArrayElement(A->Get(), It) };
            }

            inline TConstIterator& operator++() {
                ++It;
                return *this;
            }

            inline TConstIterator operator++(int) {
                TConstIterator it(*this);
                ++(*this);
                return it;
            }

            friend inline bool operator==(const TConstIterator& l, const TConstIterator& r) noexcept {
                return l.It == r.It;
            }

            friend inline bool operator!=(const TConstIterator& l, const TConstIterator& r) noexcept {
                return l.It != r.It;
            }
        };

        inline size_t Size() const noexcept {
            return TTraits::ArraySize(this->Get());
        }

        inline bool Empty() const noexcept {
            return !this->Size();
        }

        inline typename TTraits::TConstValueRef ByIndex(size_t n) const {
            return TTraits::ArrayElement(this->Get(), n);
        }

        inline typename T::TConst operator[](size_t n) const {
            return typename T::TConst {this->ByIndex(n)};
        }

        inline TConstIterator begin() const {
            return {this, TTraits::ArrayBegin(this->Get())};
        }

        inline TConstIterator end() const {
            return {this, TTraits::ArrayEnd(this->Get())};
        }

        template <typename THelper = std::nullptr_t>
        bool Validate(const TString& path, bool strict, const TErrorCallbackEx& onError, THelper helper = THelper()) const {
            if (!TTraits::IsNull(this->Value__) && !TTraits::IsArray(this->Value__)) {
                if (onError) {
                    onError(path, TString("is not an array"), TValidateInfo());
                }
                return false;
            }
            bool ok = true;
            for (size_t i = 0; i < Size(); ++i) {
                if (!(*this)[i].Validate(path + "/" + ToString(i), strict, onError, helper)) {
                    ok = false;
                };
            }
            return ok;
        }

        bool Validate(const TString& path, bool strict, const TErrorCallback& onError) {
            return Validate(path, strict, [&onError](TString p, TString e, TValidateInfo) { if (onError) { onError(p, e); } });
        }
    };

    template <typename TTraits, typename T>
    struct TConstArray : public TConstArrayImpl<typename TTraits::TConstValueRef, TTraits, T> {
        using TBase = TConstArrayImpl<typename TTraits::TConstValueRef, TTraits, T>;
        using TConst = TConstArray<TTraits, T>;

        explicit TConstArray(typename TTraits::TConstValueRef value)
            : TBase(value)
        {
        }

        template <typename U>
        TConstArray(typename TTraits::TConstValueRef value, std::initializer_list<U> def)
            : TBase(value, def)
        {
        }
    };

    template <typename TTraits, typename T>
    struct TArray : public TConstArrayImpl<typename TTraits::TValueRef, TTraits, T> {
        using TBase = TConstArrayImpl<typename TTraits::TValueRef, TTraits, T>;
        using TConst = TConstArray<TTraits, T>;

        explicit TArray(typename TTraits::TValueRef value)
            : TBase(value)
        {
        }

        template <typename U>
        TArray(typename TTraits::TValueRef value, std::initializer_list<U> def)
            : TBase(value, def)
        {
        }

        TArray(const TArray& rhs) = default;

        using TBase::Size;
        using TBase::Empty;
        using TBase::ByIndex;
        using TBase::operator[];

        typename TTraits::TValueRef GetMutable() {
            return this->Value__;
        }

        inline size_t Size() noexcept {
            return TTraits::ArraySize(this->GetMutable());
        }

        inline bool Empty() noexcept {
            return !this->Size();
        }

        inline typename TTraits::TValueRef ByIndex(size_t n) {
            return TTraits::ArrayElement(this->GetMutable(), n);
        }

        inline T operator[](size_t n) {
            return T {this->ByIndex(n)};
        }

        T Add() {
            return (*this)[this->Size()];
        }

        void Clear() {
            TTraits::ArrayClear(this->GetMutable());
        }

        template <typename TOtherArray>
        TArray& Assign(const TOtherArray& rhs) {
            Clear();
            for (auto val : rhs) {
                Add() = val;
            }
            return *this;
        }

        template <typename U>
        TArray& operator= (std::initializer_list<U> rhs) {
            return Assign(rhs);
        }

        template <typename TOtherArray>
        TArray& operator= (const TOtherArray& rhs) {
            return Assign(rhs);
        }

        TArray& operator= (const TArray& rhs) {
            return Assign(rhs);
        }
    };

    template<typename TKey, typename TStringType>
    struct TKeyToString {
        TString Value;

        TKeyToString(TKey key) {
            Value = ToString(key);
        }
    };

    template<>
    struct TKeyToString<TStringBuf, TStringBuf> {
        TStringBuf Value;
    };

    template<>
    struct TKeyToString<TString, TStringBuf> {
        TStringBuf Value;
    };

    template<>
    struct TKeyToString<TString, TString> {
        TStringBuf Value;
    };

    template <typename TKey, typename TValueRef, typename TTraits, typename T>
    struct TConstDictImpl : public TValueCommon<TValueRef, TTraits> {
        using TBase = TValueCommon<TValueRef, TTraits>;

        explicit TConstDictImpl(TValueRef value)
            : TBase(value)
        {
        }

        struct TConstIterator {
            const TConstDictImpl* A;
            typename TTraits::TDictIterator It;

            inline TConstIterator& operator++() {
                ++It;
                return *this;
            }

            inline TConstIterator operator++(int) {
                TConstIterator it(*this);
                ++(*this);
                return it;
            }

            friend inline bool operator==(const TConstIterator& l, const TConstIterator& r) noexcept {
                return l.It == r.It;
            }

            friend inline bool operator!=(const TConstIterator& l, const TConstIterator& r) noexcept {
                return l.It != r.It;
            }

            inline const TConstIterator& operator*() {
                return *this;
            }

            inline TKey Key() const {
                return FromString<TKey>(TTraits::DictIteratorKey(A->Value__, It));
            }

            inline typename T::TConst Value() const {
                return typename T::TConst { TTraits::DictIteratorValue(A->Value__, It) };
            }
        };

        inline size_t Size() const noexcept {
            return TTraits::DictSize(this->Value__);
        }

        inline bool Empty() const noexcept {
            return !this->Size();
        }

        inline typename T::TConst operator[](TKey key) const {
            TKeyToString<TKey, TTraits> keyToString {key};
            return typename T::TConst { TTraits::DictElement(this->Value__, keyToString.Value) };
        }

        inline TConstIterator begin() const {
            return {this, TTraits::DictBegin(this->Value__)};
        }

        inline TConstIterator end() const {
            return {this, TTraits::DictEnd(this->Value__)};
        }

        template <typename THelper = std::nullptr_t>
        bool Validate(const TString& path, bool strict, const TErrorCallbackEx& onError, THelper helper = THelper()) const {
            if (!TTraits::IsNull(this->Value__) && !TTraits::IsDict(this->Value__)) {
                if (onError) {
                    onError(path, TString("is not an array"), TValidateInfo());
                }
                return false;
            }
            bool ok = true;
            typename TTraits::TDictIterator it = TTraits::DictBegin(this->Value__);
            typename TTraits::TDictIterator end = TTraits::DictEnd(this->Value__);
            for (; it != end; ++it) {
                TKey key;
                if (!TryFromStringWrapper<TKey>(TTraits::DictIteratorKey(this->Value__, it), key)) {
                    ok = false;
                    if (onError) {
                        onError(path + "/" + TTraits::DictIteratorKey(this->Value__, it), TString("has invalid key type. ") + TPrimitiveTypeTraits<TKey>::TypeName() + " expected", TValidateInfo());
                    }
                }
                typename T::TConst val {TTraits::DictIteratorValue(this->Value__, it)};
                if (!val.Validate(path + "/" + TTraits::DictIteratorKey(this->Value__, it), strict, onError, helper)) {
                    ok = false;
                }
            }
            return ok;
        }

        bool Validate(const TString& path, bool strict, const TErrorCallback& onError) const {
            return Validate(path, strict, [&onError](TString p, TString e, TValidateInfo) { if (onError) { onError(p, e); } });
        }
    };

    template <typename TTraits, typename TKey, typename T>
    struct TConstDict : public TConstDictImpl<TKey, typename TTraits::TConstValueRef, TTraits, T> {
        using TBase = TConstDictImpl<TKey, typename TTraits::TConstValueRef, TTraits, T>;
        using TConst = TConstDict<TTraits, TKey, T>;

        explicit TConstDict(typename TTraits::TConstValueRef value)
            : TBase(value)
        {
        }
    };

    template <typename TTraits, typename TKey, typename T>
    struct TDict : public TConstDictImpl<TKey, typename TTraits::TValueRef, TTraits, T> {
        using TBase = TConstDictImpl<TKey, typename TTraits::TValueRef, TTraits, T>;
        using TConst = TConstDict<TTraits, TKey, T>;

        explicit TDict(typename TTraits::TValueRef value)
            : TBase(value)
        {
        }

        TDict(const TDict& rhs) = default;

        void Clear() {
            TTraits::DictClear(this->Value__);
        }

        template <typename TOtherDict>
        TDict& Assign(const TOtherDict& rhs) {
            Clear();
            for (auto val : rhs) {
                (*this)[val.Key()] = val.Value();
            }
            return *this;
        }

        template <typename TOtherDict>
        TDict& operator= (const TOtherDict& rhs) {
            return Assign(rhs);
        }

        TDict& operator= (const TDict& rhs) {
            return Assign(rhs);
        }

        inline T operator[](TKey key) {
            TKeyToString<TKey, TTraits> keyToString {key};
            return T {TTraits::DictElement(this->Value__, keyToString.Value)};
        }
    };

    template <typename TValueRef, typename TTraits>
    struct TAnyValueImpl : TValueCommon<TValueRef, TTraits> {
        using TBase = TValueCommon<TValueRef, TTraits>;
        bool WithDefault;
        TDefaultValue<TTraits> Default;

        TAnyValueImpl(TValueRef value)
            : TBase(value)
            , WithDefault(false)
        {
        }

        template <typename T>
        TAnyValueImpl(TValueRef value, T&& def)
            : TBase(value)
            , WithDefault(true)
            , Default(std::forward<T>(def))
        {
        }

        typename TTraits::TConstValueRef Get() const {
            if (WithDefault) {
                return Default.Or(this->Value__);
            } else {
                return this->Value__;
            }
        }

        operator typename TTraits::TConstValueRef() const {
            return this->Get();
        }

        typename TTraits::TConstValueRef operator->() const {
            return this->Get();
        }

        template <typename THelper = std::nullptr_t>
        bool Validate(const TString& /*path*/, bool /*strict*/, const TErrorCallbackEx& /*onError*/, THelper /*helper*/ = THelper()) const {
            return true;
        }

        bool Validate(const TString& /*path*/, bool /*strict*/, const TErrorCallback& /*onError*/) const {
            return true;
        }

        bool IsArray() const {
            return TTraits::IsArray(this->Get());
        }

        bool IsDict() const {
            return TTraits::IsDict(this->Get());
        }

        bool IsString() const {
            return this->IsPrimitive<typename TTraits::TStringType>();
        }

        template <typename T>
        bool IsPrimitive() const {
            T type{};
            return TTraitsWrapper<TTraits, TValueRef, T>::IsValidPrimitive(type, this->Get());
        }

        template <typename T>
        TConstArray<TTraits, typename TPrimitiveTypeTraits<T>::template TWrappedType<TTraits>> AsArray() const {
            return TConstArray<TTraits, typename TPrimitiveTypeTraits<T>::template TWrappedType<TTraits>>(this->Get());
        }

        template <typename TKey, typename TValue>
        TConstDict<TTraits, TKey, typename TPrimitiveTypeTraits<TValue>::template TWrappedType<TTraits>> AsDict() const {
            return TConstDict<TTraits, TKey, typename TPrimitiveTypeTraits<TValue>::template TWrappedType<TTraits>>(this->Get());
        }

        TConstPrimitive<TTraits, typename TTraits::TStringType> AsString() const {
            return this->AsPrimitive<typename TTraits::TStringType>();
        }

        template <typename T>
        TConstPrimitive<TTraits, T> AsPrimitive() const {
            if (WithDefault) {
                T defaultValue;
                TTraitsWrapper<TTraits, typename TTraits::TConstValueRef, T>::Get(Default.Or(this->Value__), defaultValue);
                return TConstPrimitive<TTraits, T>(this->Value__, std::move(defaultValue));
            } else {
                return TConstPrimitive<TTraits, T>(this->Value__);
            }
        }
    };

    template <typename TTraits>
    struct TConstAnyValue : public TAnyValueImpl<typename TTraits::TConstValueRef, TTraits> {
        using TBase = TAnyValueImpl<typename TTraits::TConstValueRef, TTraits>;
        using TConst = TConstAnyValue<TTraits>;

        TConstAnyValue(typename TTraits::TConstValueRef value)
            : TBase(value)
        {
        }

        template <typename T>
        TConstAnyValue(typename TTraits::TConstValueRef value, T&& def)
            : TBase(value, std::forward<T>(def))
        {
        }
    };

    template <typename TTraits>
    struct TAnyValue : public TAnyValueImpl<typename TTraits::TValueRef, TTraits> {
        using TBase = TAnyValueImpl<typename TTraits::TValueRef, TTraits>;
        using TConst = TConstAnyValue<TTraits>;

        TAnyValue(typename TTraits::TValueRef value)
            : TBase(value)
        {
        }

        template <typename T>
        TAnyValue(typename TTraits::TValueRef value, T&& def)
            : TBase(value, std::forward<T>(def))
        {
        }

        TAnyValue(const TAnyValue& rhs) = default;

        typename TTraits::TValueRef GetMutable() {
            return this->Value__;
        }

        operator typename TTraits::TValueRef() {
            return this->GetMutable();
        }

        typename TTraits::TValueRef operator->() {
            return this->GetMutable();
        }

        template <typename T>
        TAnyValue& Assign(const T& rhs) {
            return AssignImpl(rhs, TDummy{});
        }

    private:
        struct TDummy{};

        template <typename T>
        TAnyValue& AssignImpl(const T& rhs, ...) {
            TTraitsWrapper<TTraits, typename TTraits::TValueRef, T>::Set(this->GetMutable(), rhs);
            return *this;
        }

        template <typename T, std::enable_if_t<std::is_convertible<T, typename TTraits::TStringType>::value>* = nullptr>
        TAnyValue& AssignImpl(const T& rhs, TDummy) {
            TTraits::Set(this->GetMutable(), static_cast<typename TTraits::TStringType>(rhs));
            return *this;
        }

        Y_HAS_MEMBER(Get);

        template <typename TOtherAnyValue, std::enable_if_t<THasGet<TOtherAnyValue>::value>* = nullptr>
        TAnyValue& AssignImpl(const TOtherAnyValue& rhs, TDummy) {
            *GetMutable() = *rhs.Get();
            return *this;
        }

    public:
        template <typename T>
        TAnyValue& operator= (const T& rhs) {
            return Assign(rhs);
        }

        TAnyValue& operator= (const TAnyValue& rhs) {
            return Assign(rhs);
        }
    };

    // checks that levenshtein distance is <= 1
    inline bool IsSimilar(TStringBuf s1, TStringBuf s2) {
        size_t minLen = Min(s1.size(), s2.size());
        size_t prefixLen = 0;
        while (prefixLen < minLen && s1[prefixLen] == s2[prefixLen]) {
            ++prefixLen;
        }
        return
            s1.SubStr(prefixLen+1) == s2.SubStr(prefixLen+1) ||
            s1.SubStr(prefixLen+0) == s2.SubStr(prefixLen+1) ||
            s1.SubStr(prefixLen+1) == s2.SubStr(prefixLen+0);
    }
}

#endif
