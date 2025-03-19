#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/fwd.h>
#include <util/system/type_name.h>

namespace NJson {
    template <class T>
    TJsonValue ToJson(const T& object);

    template <class T>
    TMaybe<T> TryFromJson(const TJsonValue& value);

    template <class T>
    bool TryFromJson(const TJsonValue& value, T& result);

    template <class T>
    T FromJson(const TJsonValue& value);

    template <class T>
    T FromJsonWithDefault(const TJsonValue& value, const T& def);
}

template <class T>
TMaybe<T> NJson::TryFromJson(const TJsonValue& value) {
    T result;
    if (TryFromJson(value, result)) {
        return result;
    } else {
        return {};
    }
}

template <class T>
T NJson::FromJson(const NJson::TJsonValue& value) {
    T result;
    Y_ENSURE(TryFromJson(value, result), "cannot parse " << value.GetStringRobust() << " as " << TypeName<T>());
    return result;
}

template <class T>
T NJson::FromJsonWithDefault(const NJson::TJsonValue& value, const T& def) {
    T result;
    if (TryFromJson(value, result)) {
        return result;
    } else {
        return def;
    }
}

namespace NJson {
    namespace NPrivate {
        template <class T>
        void TupleToJson(TJsonValue& result, const T& element);
        template <class T, class... TArgs>
        void TupleToJson(TJsonValue& result, const T& element, const TArgs&... rest) {
            NPrivate::TupleToJson(result, element);
            NPrivate::TupleToJson(result, rest...);
        }

        template <class T>
        bool TryTupleFromJson(TJsonValue::TArray::const_iterator i, TJsonValue::TArray::const_iterator end, T& element);
        template <class T, class... TArgs>
        bool TryTupleFromJson(TJsonValue::TArray::const_iterator i, TJsonValue::TArray::const_iterator end, T& element, TArgs&... args) {
            if (NPrivate::TryTupleFromJson(i, end, element)) {
                return NPrivate::TryTupleFromJson(i + 1, end, args...);
            } else {
                return false;
            }
        }
    }

    template <class T>
    TJsonValue TupleToJson(const T& object) {
        TJsonValue result;
        std::apply([&result](auto&&... args) {
            NPrivate::TupleToJson(result, args...);
        }, object);
        return result;
    }
    template <class T>
    bool TryTupleFromJson(const TJsonValue& value, T& result) {
        if (!value.IsArray()) {
            return false;
        }
        const TJsonValue::TArray& a = value.GetArray();
        return std::apply([&a](auto&&... args) {
            return NPrivate::TryTupleFromJson(a.begin(), a.end(), args...);
        }, std::forward<T>(result));
    }

    template <class T1, class T2>
    TJsonValue ToJson(const std::pair<T1, T2>& object) {
        return TupleToJson(object);
    }
    template <class... TArgs>
    TJsonValue ToJson(const std::tuple<TArgs...>& object) {
        return TupleToJson(object);
    }

    template <class T1, class T2>
    bool TryFromJson(const TJsonValue& value, std::pair<T1, T2>& result) {
        return TryTupleFromJson(value, result);
    }
    template <class... TArgs>
    bool TryFromJson(const TJsonValue& value, std::tuple<TArgs...>& result) {
        return TryTupleFromJson(value, result);
    }
    template <class... TArgs>
    bool TryFromJson(const TJsonValue& value, std::tuple<TArgs...>&& result) {
        return TryTupleFromJson(value, result);
    }

    template <class T>
    TJsonValue PointerToJson(const T& object);

    template <class T, class P>
    TJsonValue ToJson(const TMaybe<T, P>& object) {
        return PointerToJson(object);
    }
    template <class T, class D>
    TJsonValue ToJson(const THolder<T, D>& object) {
        return PointerToJson(object);
    }
    template <class T, class C, class D>
    TJsonValue ToJson(const TSharedPtr<T, C, D>& object) {
        return PointerToJson(object);
    }
    template <class T, class Ops>
    TJsonValue ToJson(const TIntrusivePtr<T, Ops>& object) {
        return PointerToJson(object);
    }
    template <class T, class Ops>
    TJsonValue ToJson(const TIntrusiveConstPtr<T, Ops>& object) {
        return PointerToJson(object);
    }

    template <class I>
    TJsonValue RangeToJson(I&& begin, I&& end);

    template <class T, class C, class A>
    TJsonValue ToJson(const TSet<T, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class T, class C, class A>
    TJsonValue ToJson(const TMultiSet<T, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class T, class C, class A>
    TJsonValue ToJson(const THashSet<T, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class T, class C, class A>
    TJsonValue ToJson(const THashMultiSet<T, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }

    template <class K, class V, class C, class A>
    TJsonValue ToJson(const TMap<K, V, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class K, class V, class C, class A>
    TJsonValue ToJson(const TMultiMap<K, V, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class K, class V, class C, class A>
    TJsonValue ToJson(const THashMap<K, V, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class K, class V, class C, class A>
    TJsonValue ToJson(const THashMultiMap<K, V, C, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }

    template <class T, class A>
    TJsonValue ToJson(const TVector<T, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class T, class A>
    TJsonValue ToJson(const TDeque<T, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class T, class A>
    TJsonValue ToJson(const TList<T, A>& container) {
        return RangeToJson(container.begin(), container.end());
    }
    template <class T, size_t N>
    TJsonValue ToJson(const std::array<T, N>& container) {
        return RangeToJson(container.begin(), container.end());
    }

    template <class T, class P>
    bool TryFromJson(const TJsonValue& value, TMaybe<T, P>& result) {
        if (!value.IsDefined()) {
            result.Clear();
            return true;
        }

        T v;
        if (TryFromJson(value, v)) {
            result = std::move(v);
            return true;
        } else {
            return false;
        }
    }

    template <class TContainer>
    bool TryPushBackableFromJson(const TJsonValue& value, TContainer& result) {
        TContainer container;
        if (!value.IsArray()) {
            return false;
        }
        container.reserve(value.GetArray().size());
        for (auto&& i : value.GetArray()) {
            if (!TryFromJson(i, container.emplace_back())) {
                return false;
            }
        }
        result = std::move(container);
        return true;
    }
    template <class T, class TContainer>
    bool TryInsertableFromJson(const TJsonValue& value, TContainer& result) {
        TContainer container;
        if (!value.IsArray()) {
            return false;
        }
        for (auto&& i : value.GetArray()) {
            T element;
            if (!TryFromJson(i, element)) {
                return false;
            }
            container.insert(std::move(element));
        }
        result = std::move(container);
        return true;
    }

    template <class T, class C, class A>
    bool TryFromJson(const TJsonValue& value, TSet<T, C, A>& result) {
        return TryInsertableFromJson<T>(value, result);
    }
    template <class T, class C, class A>
    bool TryFromJson(const TJsonValue& value, TMultiSet<T, C, A>& result) {
        return TryInsertableFromJson<T>(value, result);
    }
    template <class T, class C, class A>
    bool TryFromJson(const TJsonValue& value, THashSet<T, C, A>& result) {
        return TryInsertableFromJson<T>(value, result);
    }
    template <class T, class C, class A>
    bool TryFromJson(const TJsonValue& value, THashMultiSet<T, C, A>& result) {
        return TryInsertableFromJson<T>(value, result);
    }

    template <class K, class V, class C, class A>
    bool TryFromJson(const TJsonValue& value, TMap<K, V, C, A>& result) {
        return TryInsertableFromJson<std::pair<K, V>>(value, result);
    }
    template <class K, class V, class C, class A>
    bool TryFromJson(const TJsonValue& value, TMultiMap<K, V, C, A>& result) {
        return TryInsertableFromJson<std::pair<K, V>>(value, result);
    }
    template <class K, class V, class C, class A>
    bool TryFromJson(const TJsonValue& value, THashMap<K, V, C, A>& result) {
        return TryInsertableFromJson<std::pair<K, V>>(value, result);
    }
    template <class K, class V, class C, class A>
    bool TryFromJson(const TJsonValue& value, THashMultiMap<K, V, C, A>& result) {
        return TryInsertableFromJson<std::pair<K, V>>(value, result);
    }

    template <class T, class A>
    bool TryFromJson(const TJsonValue& value, TVector<T, A>& result) {
        return TryPushBackableFromJson(value, result);
    }
    template <class T, class A>
    bool TryFromJson(const TJsonValue& value, TDeque<T, A>& result) {
        return TryPushBackableFromJson(value, result);
    }
    template <class T, class A>
    bool TryFromJson(const TJsonValue& value, TList<T, A>& result) {
        return TryPushBackableFromJson(value, result);
    }
}

namespace NJson {
    namespace NPrivate {
        template <class T>
        void TupleToJson(TJsonValue& result, const T& element) {
            result.AppendValue(ToJson(element));
        }

        template <class T>
        bool TryTupleFromJson(TJsonValue::TArray::const_iterator i, TJsonValue::TArray::const_iterator end, T& element) {
            if (i == end) {
                return false;
            }
            return TryFromJson(*i, element);
        }
    }

    template <class T>
    TJsonValue PointerToJson(const T& object) {
        if (object) {
            return ToJson(*object);
        } else {
            return JSON_NULL;
        }
    }

    template <class I>
    TJsonValue RangeToJson(I&& begin, I&& end) {
        TJsonValue result = JSON_ARRAY;
        for (auto i = begin; i != end; ++i) {
            result.AppendValue(ToJson(*i));
        }
        return result;
    }
}
