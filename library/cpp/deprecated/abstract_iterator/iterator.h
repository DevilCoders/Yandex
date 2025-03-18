#pragma once

#include <cassert>

#include <util/generic/vector.h>
#include <util/stream/tokenizer.h>
#include <util/generic/noncopyable.h>

template <typename T>
class IIterator {
public:
    typedef T TValueType;

    virtual bool Eof() const = 0;
    virtual void Move() = 0;
    virtual void NowInplace(T& result) const = 0;

    virtual inline T Now() const {
        T result;
        NowInplace(result);
        return result;
    }

    virtual inline void NextInplace(T& result) {
        NowInplace(result);
        Move();
    }

    virtual inline T Next() {
        T result;
        NowInplace(result);
        Move();
        return result;
    }

    inline bool operator<(const IIterator& rhs) const {
        return Now() < rhs.Now();
    }

    virtual ~IIterator() = default;
    ;
};

template <typename T>
class TItCmp {
public:
    inline bool operator()(const T* a, const T* b) {
        return *b < *a;
    }
};

template <typename T>
class TRangeIterator: public IIterator<T> {
private:
    T Min;
    T Max;
    T Step;
    T Now;

public:
    TRangeIterator(T max)
        : Min(0)
        , Max(max)
        , Step(1)
        , Now(Min)
    {
    }

    TRangeIterator(T min, T max)
        : Min(min)
        , Max(max)
        , Step(1)
        , Now(Min)
    {
    }

    TRangeIterator(T min, T max, T step)
        : Min(min)
        , Max(max)
        , Step(step)
        , Now(Min)
    {
    }

    bool Eof() const override {
        return Now >= Max;
    }

    void Move() override {
        assert(!Eof());
        Now += Step;
    }

    void NowInplace(T& result) const override {
        result = Now;
    }
};

template <typename T>
TRangeIterator<T> GetRangeIterator(T max) {
    return TRangeIterator<T>(max);
}

template <typename T>
class TContIterator: public IIterator<typename T::value_type> {
private:
    typedef typename T::const_iterator TIt;
    TIt Now;
    TIt Begin;
    TIt End;

public:
    TContIterator(const T& cont)
        : Now(cont.begin())
        , Begin(cont.begin())
        , End(cont.end())
    {
    }

    bool Eof() const override {
        return Now == End;
    }

    void Move() override {
        ++Now;
    }

    void NowInplace(typename T::value_type& result) const override {
        result = *Now;
    }
};

template <typename T>
TContIterator<T> GetContIterator(const T& cont) {
    return TContIterator<T>(cont);
}

template <typename T>
void Fill(IIterator<T>& it, TVector<T>& result) {
    while (!it.Eof())
        result.push_back(it.Next());
}

struct TStrRegion {
    char* Str;
    size_t Len;
};

struct TEoln {
    Y_FORCE_INLINE bool operator()(char ch) {
        return '\n' == ch;
    }
};

class TFileLineIterator: public IIterator<const TStrRegion*>, TNonCopyable {
private:
    typedef TStreamTokenizer<TEoln> TTokenizer;
    TTokenizer Tokenizer;

    bool IsEof;
    TStrRegion Part;
    size_t Line;

    inline void TryNext() {
        ++Line;
        IsEof = !Tokenizer.Next(Part.Str, Part.Len);
    }

protected:
    const TStrRegion& GetRegion() const {
        return Part;
    }

public:
    TFileLineIterator(IInputStream* input)
        : Tokenizer(input)
        , IsEof(false)
        , Line(0)
    {
        TryNext();
    }

    inline bool Eof() const override {
        return IsEof;
    }

    void Move() override {
        assert(!Eof());
        TryNext();
    }

    void NowInplace(const TStrRegion*& result) const override {
        assert(!Eof());
        result = &Part;
    }

    size_t GetLine() const {
        return Line;
    }
};
