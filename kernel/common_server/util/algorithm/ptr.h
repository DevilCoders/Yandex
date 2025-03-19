#pragma once

#include <library/cpp/logger/global/global.h>

#include <util/generic/bt_exception.h>
#include <util/system/type_name.h>

template <class T, class U, class C, class D>
Y_WARN_UNUSED_RESULT TSharedPtr<T, C, D> DynamicPointerCast(const TSharedPtr<U, C, D>& other) noexcept {
    auto p = dynamic_cast<T*>(other.Get());
    if (p) {
        auto c = other.ReferenceCounter();
        if (c) {
            c->Inc();
        }
        return { p, c };
    } else {
        return nullptr;
    }
}

namespace std {
    template <class T, class U, class C, class D>
    Y_WARN_UNUSED_RESULT TSharedPtr<T, C, D> dynamic_pointer_cast(const TSharedPtr<U, C, D>& other) noexcept {
        auto p = dynamic_cast<T*>(other.Get());
        if (p) {
            auto c = other.ReferenceCounter();
            if (c) {
                c->Inc();
            }
            return { p, c };
        } else {
            return nullptr;
        }
    }

    template <class T, class U, class D>
    Y_WARN_UNUSED_RESULT THolder<T, D> dynamic_pointer_cast(THolder<U, D>& other) noexcept {
        auto p = dynamic_cast<T*>(other.Get());
        if (p) {
            (void) other.Release();
            return THolder(p);
        } else {
            return nullptr;
        }
    }

    template <class T, class U, class D>
    Y_WARN_UNUSED_RESULT THolder<T, D> dynamic_pointer_cast(THolder<U, D>&& other) noexcept {
        return dynamic_pointer_cast<T, U, D>(other);
    }
}

template <class T>
inline T&& Checked(T&& t) noexcept {
    CHECK_WITH_LOG(!!t) << TypeName<decltype(*t)>() << " is null";
    return std::forward<T>(t);
}

template <class T>
inline T&& Yasserted(T&& t) noexcept {
    Y_ASSERT(!!t);
    return std::forward<T>(t);
}

template <class T>
inline T&& Yensured(T&& t) {
    Y_ENSURE_EX(!!t, TWithBackTrace<yexception>() << TypeName<decltype(*t)>() << " is null");
    return std::forward<T>(t);
}

template <class T>
Y_WARN_UNUSED_RESULT THolder<T> Hold(T* t) noexcept {
    return t;
}

template <class T, class D>
Y_WARN_UNUSED_RESULT TAtomicSharedPtr<T, D> Share(THolder<T, D>&& t) noexcept {
    return std::move(t);
}
