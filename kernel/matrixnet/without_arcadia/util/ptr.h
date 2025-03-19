#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <memory>

namespace NMatrixnet {

template<typename T>
class THolder : public std::unique_ptr<T> {
public:
    using TBase = std::unique_ptr<T>;
    THolder(T* ptr) : TBase(ptr) {}
    T* Get() { return TBase::get(); }
    const T* Get() const { return TBase::get(); }
};

template<typename T>
class TArrayHolder : public std::unique_ptr<T[]> {
public:
    using TBase = std::unique_ptr<T[]>;
    TArrayHolder(T* ptr) : TBase(ptr) {}
    T* Get() { return TBase::get(); }
    const T* Get() const { return TBase::get(); }
};

template<typename T>
using TAtomicSharedPtr = std::shared_ptr<T>;

} // namespace NMatrixnet
