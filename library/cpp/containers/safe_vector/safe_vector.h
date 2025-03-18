#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <util/system/yassert.h>
#include <util/ysaveload.h>

template <class T, class A = std::allocator<T>>
class TSafeVector: private TVector<T, A> {
public:
    using TBase = TVector<T, A>;

    using typename TBase::const_iterator;
    using typename TBase::const_reference;
    using typename TBase::iterator;
    using typename TBase::reference;
    using typename TBase::reverse_iterator;
    using typename TBase::size_type;
    using typename TBase::value_type;

    using TBase::begin;
    using TBase::end;
    using TBase::rbegin;
    using TBase::rend;

    using TBase::empty;
    using TBase::size;
    using TBase::ysize;

    using TBase::assign;
    using TBase::clear;
    using TBase::push_back;
    using TBase::emplace_back;
    using TBase::reserve;
    using TBase::resize;

    using TBase::operator bool;

    TSafeVector() = default;

    TSafeVector(size_type n, const value_type& v = value_type())
        : TBase(n, v)
    {
    }

    template <class I>
    TSafeVector(I b, I e)
        : TBase(b, e)
    {
    }

    TSafeVector(const TSafeVector& v)
        : TBase(v)
    {
    }

    TSafeVector(TSafeVector&& v)
        : TBase(v)
    {
    }

    TSafeVector& operator=(const TSafeVector& v) {
        TBase::operator=(v);
        return *this;
    }

    TSafeVector& operator=(TSafeVector&& v) noexcept {
        TBase::operator=(v);
        return *this;
    }

    reference front() {
        CheckItem();
        return TBase::front();
    }

    const_reference front() const {
        CheckItem();
        return TBase::front();
    }

    reference back() {
        CheckItem();
        return TBase::back();
    }

    const_reference back() const {
        CheckItem();
        return TBase::back();
    }

    reference operator[](size_type n) {
        CheckItem(n);
        return *(begin() + n);
    }

    const_reference operator[](size_type n) const {
        CheckItem(n);
        return *(begin() + n);
    }

    reference at(size_type n) {
        CheckItem(n);
        return *(begin() + n);
    }

    const_reference at(size_type n) const {
        CheckItem(n);
        return *(begin() + n);
    }

    iterator insert(iterator position, const value_type& val) {
        CheckIter(position);
        return TBase::insert(position, val);
    }

    void insert(iterator position, size_type n, const value_type& val) {
        CheckIter(position);
        TBase::insert(position, n, val);
    }

    template <class I>
    void insert(iterator position, I first, I last) {
        CheckIter(position);
        TBase::insert(position, first, last);
    }

    iterator erase(iterator position) {
        CheckIter(position);
        return TBase::erase(position);
    }

    iterator erase(iterator first, iterator last) {
        CheckIter(first);
        CheckIter(last);
        Y_ENSURE(first <= last, "invalid input iterators");
        return TBase::erase(first, last);
    }

    void pop_back() {
        if (empty()) {
            ythrow yexception() << TStringBuf("using pop_back() for empty vector") << Endl;
        }
        TBase::pop_back();
    }

    void swap(TSafeVector& other) {
        TBase::swap(other);
    }

    TBase& AsVector() {
        return *this;
    }

    const TBase& AsVector() const {
        return *this;
    }

private:
    void CheckItem(size_type n = 0) const {
        Y_ENSURE(n < size(),
                TStringBuilder() << "invalid vector index: using " << n
                                 << ", but size() is equal to )" << size()
        );
    }

    void CheckIter(iterator it) const {
        Y_ENSURE(it >= begin(), "invalid iterator for vector using");
        Y_ENSURE(it <= end(), "invalid iterator for vector using");
    }
};

template <class T, class A>
class TSerializer<TSafeVector<T, A>>: public TVectorSerializer<TSafeVector<T, A>> {
};
