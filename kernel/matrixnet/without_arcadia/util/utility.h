#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <algorithm>
#include <string>

template <class T>
static const T& Min(const T& l, const T& r) {
    return std::min(l, r);
}

template <class T>
static const T& Max(const T& l, const T& r) {
    return std::max(l, r);
}

template<typename T>
struct SwapImpl {
    static void Swap(T& a, T& b) {
        a.Swap(b);
    }
};

template<typename T>
void DoSwap(T& a, T& b) {
    SwapImpl<T>::Swap(a, b);
}

template<typename T>
struct STLSwapImpl {
    static void Swap(T& a, T& b) {
        std::swap(a, b);
    }
};

template<> struct SwapImpl<bool> : public STLSwapImpl<bool> {};
template<> struct SwapImpl<uint32_t> : public STLSwapImpl<uint32_t> {};
template<> struct SwapImpl<int64_t> : public STLSwapImpl<int64_t> {};
template<> struct SwapImpl<size_t> : public STLSwapImpl<size_t> {};
template<> struct SwapImpl<double> : public STLSwapImpl<double> {};
template<> struct SwapImpl<std::string> : public STLSwapImpl<std::string> {};

template<typename T>
struct SwapImpl<const T*> : public STLSwapImpl<const T*> {};

template<typename T>
struct SwapImpl<T*> : public STLSwapImpl<T*> {};
