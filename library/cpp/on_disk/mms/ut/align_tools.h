#pragma once

#include <contrib/libs/mms/impl/container.h>

#include <util/system/defaults.h>
#include <util/generic/typetraits.h>

// important: class name is not TTestAccess because class TestAccess is declared friend inside contrib/libs/mms
struct TestAccess {
    static inline const ssize_t& Offset(const mms::impl::Container& c) {
        return c.ofs_.offset_;
    }

    static inline const size_t& Size(const mms::impl::Container& c) {
        return c.size_;
    }
};

template <class C, class K, class I, class Cmp>
bool IsAligned(const mms::impl::SortedSequence<C, K, I, Cmp>& v);

template <class T>
bool IsAligned(const mms::impl::Sequence<T>& v);

template <class T>
inline std::enable_if_t<
    sizeof(std::remove_reference_t<T>) == sizeof(size_t),
    bool>
IsAligned(T ptr) {
    return reinterpret_cast<size_t>(&ptr) % sizeof(void*) == 0;
}

template <class T>
inline std::enable_if_t<
    sizeof(std::remove_reference_t<T>) != sizeof(size_t) && TTypeTraits<std::remove_reference_t<T>>::IsPod,
    bool>
IsAligned(T) {
    return true;
}

template <class F, class S>
bool IsAligned(const std::pair<F, S>& p) {
    return IsAligned(p.first) && IsAligned(p.second);
}

inline bool IsAligned(const mms::impl::Container& c) {
    return IsAligned(TestAccess::Offset(c)) &&
           IsAligned(TestAccess::Size(c)) &&
           (TestAccess::Offset(c)) % sizeof(void*) == 0;
}

template <class T>
bool IsAligned(const mms::impl::Sequence<T>& v) {
    if (!IsAligned(static_cast<const mms::impl::Container&>(v))) {
        return false;
    }
    typename mms::impl::Sequence<T>::const_iterator it = v.begin();
    for (; it != v.end(); ++it) {
        if (!IsAligned(*it)) {
            return false;
        }
    }
    return true;
}

template <class K, class T, class Cmp, class XK>
bool IsAligned(const mms::impl::SortedSequence<K, T, Cmp, XK>& v) {
    if (!IsAligned(static_cast<const mms::impl::Container&>(v))) {
        return false;
    }
    typename mms::impl::SortedSequence<K, T, Cmp, XK>::const_iterator it = v.begin();
    for (; it != v.end(); ++it) {
        if (!IsAligned(*it)) {
            return false;
        }
    }
    return true;
}
