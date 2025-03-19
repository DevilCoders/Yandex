#pragma once

#include <util/system/type_name.h>
#include <util/generic/variant.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

namespace NPrivate {
    template <class E>
    struct Y_WARN_UNUSED_RESULT TUnexpected {
        E Value;
        TUnexpected(const E& value)
            : Value(value)
        {
        }
    };
}

class TUnexpectedException : public yexception {
};

template <class T, class E>
class Y_WARN_UNUSED_RESULT TExpected {
public:
    enum EPolicy {
        Throw,
        Verify,
    };

public:
    static_assert(!std::is_reference<T>::value);
    static_assert(!std::is_reference<E>::value);
    static_assert(!std::is_void<E>::value);

public:
    static constexpr EPolicy DefaultPolicy = Verify;

public:
    template <class... TArgs>
    TExpected(TArgs&&... args)
        : Value(T(std::forward<TArgs>(args)...))
    {
    }

    TExpected(const NPrivate::TUnexpected<E>& unexpected)
        : Value(unexpected.Value)
    {
    }
    TExpected(NPrivate::TUnexpected<E>& unexpected)
        : Value(unexpected.Value)
    {
    }
    TExpected(NPrivate::TUnexpected<E>&& unexpected)
        : Value(std::move(unexpected.Value))
    {
    }

    inline operator bool() const {
        return IsValue();
    }
    inline const T* operator->() const {
        return &GetValue();
    }
    inline T* operator->() {
        return &GetValue();
    }
    inline const T& operator*() const {
        return GetValue();
    }
    inline T& operator*() {
        return GetValue();
    }

    inline const E& GetError(EPolicy policy = DefaultPolicy) const {
        CheckError(policy);
        return std::get<E>(Value);
    }
    inline const T* Get() const {
        return IsValue() ? &GetValue() : nullptr;
    }
    inline T* Get() {
        return IsValue() ? &GetValue() : nullptr;
    }
    inline const T& GetValue(EPolicy policy = DefaultPolicy) const {
        CheckValue(policy);
        return std::get<T>(Value);
    }
    inline T& GetValue(EPolicy policy = DefaultPolicy) {
        CheckValue(policy);
        return std::get<T>(Value);
    }
    inline T ExtractValue(EPolicy policy = DefaultPolicy) {
        CheckValue(policy);
        return std::move(std::get<T>(Value));
    }

private:
    bool IsValue() const {
        return std::holds_alternative<T>(Value);
    }
    bool IsError() const {
        return std::holds_alternative<E>(Value);
    }
    void CheckValue(EPolicy policy) const {
        Y_ASSERT(IsError() || IsValue());
        if (IsValue()) {
            return;
        }
        switch (policy) {
        case Verify:
            if constexpr (std::is_convertible<E*, std::exception*>::value) {
                Y_FAIL("%s: encountered unexpected error %s", TypeName<T>().c_str(), GetError(policy).what());
            } else {
                Y_FAIL("%s: encountered unexpected error %s", TypeName<T>().c_str(), ToString(GetError(policy)).c_str());
            }
        case Throw:
            if constexpr (std::is_convertible<E*, std::exception*>::value) {
                throw GetError();
            } else {
                throw TUnexpectedException() << TypeName<T>() << ": encountered unexpected error " << GetError(policy);
            }
        }
    }
    void CheckError(EPolicy policy) const {
        Y_ASSERT(IsError() || IsValue());
        if (IsError()) {
            return;
        }
        switch (policy) {
        case Verify:
            Y_FAIL("%s: encountered expected value", TypeName<T>().c_str());
        case Throw:
            throw TUnexpectedException() << TypeName<T>() << ": encountered expected value";
        }
    }

private:
    std::variant<T, E> Value;
};

template <class E>
auto MakeUnexpected(E&& e) {
    return NPrivate::TUnexpected<std::decay_t<E>>(std::forward<E>(e));
}

template <class E, class F, class... TArgs>
auto WrapUnexpected(F&& f, TArgs&&... args) {
    using T = decltype(f(std::forward<TArgs>(args)...));
    try {
        auto result = f(std::forward<TArgs>(args)...);
        return TExpected<T, E>(std::move(result));
    } catch (E& e) {
        return TExpected<T, E>(MakeUnexpected(std::move(e)));
    }
}

template <class T, class E>
IOutputStream& operator<<(IOutputStream& out, const TExpected<T, E>& value) {
    if (value) {
        out << *value;
    } else {
        if constexpr (std::is_convertible<E*, std::exception*>::value) {
            out << value.GetError().what();
        } else {
            out << value.GetError();
        }
    }
    return out;
}
