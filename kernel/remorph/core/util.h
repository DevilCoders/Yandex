#pragma once

#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <util/ysaveload.h>
#include <utility>

namespace NRemorph {

template <class T>
class TConstIntrusivePtrOps {
    using TNonConst = std::remove_const_t<T>;

public:
    static inline void Ref(const T* t) noexcept {
        Y_ASSERT(t);

        const_cast<TNonConst*>(t)->Ref();
    }

    static inline void UnRef(const T* t) noexcept {
        Y_ASSERT(t);

        const_cast<TNonConst*>(t)->UnRef();
    }

    static inline void DecRef(const T* t) noexcept {
        Y_ASSERT(t);

        const_cast<TNonConst*>(t)->DecRef();
    }
};

template <typename T>
struct TSignedLimits {
    const static T Max = ~(1 << ((sizeof(T) * 8) - 1));
    const static T Min = (1 << ((sizeof(T) * 8) - 1));
};

template <typename T>
struct TUnsignedLimits {
    const static T Max = ~static_cast<T>(0);
    const static T Min = 0;
};

template <typename T>
struct TStaticLimits {
    static_assert(std::is_integral<T>::value, "T must be an integral type.");
    const static T Max = std::conditional_t<std::is_signed<T>::value, TSignedLimits<T>, TUnsignedLimits<T>>::Max;
    const static T Min = std::conditional_t<std::is_signed<T>::value, TSignedLimits<T>, TUnsignedLimits<T>>::Min;
};

template <typename TFirst, typename TSecond>
struct TCompare1st {
    Y_FORCE_INLINE bool operator() (const std::pair<TFirst, TSecond>& f, const std::pair<TFirst, TSecond>& s) const {
        return f.first == s.first;
    }
};

template <typename TContainer>
Y_FORCE_INLINE bool EqualKeys(const TContainer& f, const TContainer& s) {
    typedef typename TContainer::value_type::first_type TFirst;
    typedef typename TContainer::value_type::second_type TSecond;

    return f.size() == s.size() && Equal(f.begin(), f.end(), s.begin(), TCompare1st<TFirst, TSecond>());
}

struct TNop {
    template <typename T>
    Y_FORCE_INLINE void operator() (const T&) const {
    }

    template <typename T1, typename T2>
    Y_FORCE_INLINE void operator() (const T1&, const T2&) const {
    }

    template <typename T1, typename T2, typename T3>
    Y_FORCE_INLINE void operator() (const T1&, const T2&, const T3&) const {
    }
};

struct TTrueNop {
    template <typename T>
    Y_FORCE_INLINE bool operator() (const T&) const {
        return true;
    }
};

template <typename T>
struct TMover {
    TVector<T>& Result;
    TMover(TVector<T>& r)
        : Result(r)
    {
    }

    Y_FORCE_INLINE void operator() (T& o) const {
        Result.emplace_back();
        DoSwap(Result.back(), o);
    }
};

template <typename T>
struct TPusher {
    TVector<T>& Result;
    TPusher(TVector<T>& r)
        : Result(r)
    {
    }

    Y_FORCE_INLINE void operator() (const T& o) const {
        Result.push_back(o);
    }
};

template <typename T, typename TResult, TResult ret = TResult()>
struct TReturnPusher {
    TVector<T>& Result;
    TReturnPusher(TVector<T>& r)
        : Result(r)
    {
    }

    Y_FORCE_INLINE TResult operator() (const T& o) const {
        Result.push_back(o);
        return ret;
    }
};

template <typename T>
struct TPtrPusher {
    TVector<T>& Result;
    TPtrPusher(TVector<T>& r)
        : Result(r)
    {
    }

    template <class TPtr>
    Y_FORCE_INLINE void operator() (const TPtr& p) const {
        Result.push_back(p.Get());
    }
};

template <typename T, class TIter>
struct TRangeIterator {
    TIter Start;
    TIter End;

    TRangeIterator(TIter start, TIter end)
        : Start(start)
        , End(end)
    {
    }

    Y_FORCE_INLINE bool Ok() const {
        return Start != End;
    }

    Y_FORCE_INLINE T& operator *() const {
        Y_ASSERT(Ok());
        return *Start;
    }

    Y_FORCE_INLINE T* operator ->() const {
        Y_ASSERT(Ok());
        return &*Start;
    }

    Y_FORCE_INLINE TRangeIterator& operator ++() {
        Y_ASSERT(Ok());
        ++Start;
        return *this;
    }

    Y_FORCE_INLINE TRangeIterator operator ++(int) {
        Y_ASSERT(Ok());
        TRangeIterator res = *this;
        ++Start;
        return res;
    }
};

template <typename TContainer>
struct TMultiResultHolder {
    TContainer& Results;

    TMultiResultHolder(TContainer& res)
        : Results(res)
    {
    }

    static Y_FORCE_INLINE bool AcceptMore() {
        return true;
    }

    Y_FORCE_INLINE void Put(typename TTypeTraits<typename TContainer::value_type>::TFuncParam r) {
        Results.push_back(r);
    }
};

template <typename T>
struct TSingleResultHolder {
    T Result;

    Y_FORCE_INLINE bool AcceptMore() const {
        return !Result;
    }

    Y_FORCE_INLINE void Put(typename TTypeTraits<T>::TFuncParam r) {
        Result = r;
    }
};

} // NRemorph

template<class T>
class TSerializer<TIntrusivePtr<T>> {
    typedef TIntrusivePtr<T> TPtr;
public:
    static inline void Save(IOutputStream* rh, const TPtr& p) {
        ::Save(rh, *p);
    }

    static inline void Load(IInputStream* rh, TPtr& p) {
        TPtr t(new T());
        ::Load(rh, *t);
        p.Swap(t);
    }

    template <class TStorage>
    static inline void Load(IInputStream* rh, TPtr& p, TStorage& pool) {
        TPtr t(new T());
        ::Load(rh, *t, pool);
        p.Swap(t);
    }
};
