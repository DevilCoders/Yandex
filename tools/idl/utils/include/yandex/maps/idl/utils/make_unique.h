#pragma once

#include <cstddef>
#include <memory>
#include <utility>

namespace yandex {
namespace maps {
namespace idl {
namespace utils {

// make_unique implementation from C++14 Standard proposal
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm
namespace internal {
template<class T> struct _Unique_if {
    using _Single_object = std::unique_ptr<T>;
};

template<class T> struct _Unique_if<T[]> {
    using _Unknown_bound = std::unique_ptr<T[]>;
};

template<class T, size_t N> struct _Unique_if<T[N]> {
    using _Known_bound = void;
};
} // namespace internal

template<class T, class... Args>
typename internal::_Unique_if<T>::_Single_object make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T>
typename internal::_Unique_if<T>::_Unknown_bound make_unique(size_t n)
{
    using U = typename std::remove_extent<T>::type;
    return std::unique_ptr<T>(new U[n]());
}

template<class T, class... Args>
typename internal::_Unique_if<T>::_Known_bound make_unique(Args&&...) = delete;

} // namespace utils
} // namespace idl
} // namespace maps
} // namespace yandex
