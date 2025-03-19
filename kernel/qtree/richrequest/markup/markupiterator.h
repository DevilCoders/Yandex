#pragma once

#include "markup.h"

namespace NSearchQuery {

template <class T1, class T2>
struct TLogicalAnd {
    T1 C1;
    T2 C2;

    TLogicalAnd(const T1& c1, const T2& c2)
    : C1(c1)
    , C2(c2)
    {
    }

    bool operator () (const TMarkupItem& item) const {
        return C1(item) && C2(item);
    }
};

template <class T1, class T2>
struct TLogicalOr {
    T1 C1;
    T2 C2;

    TLogicalOr(const T1& c1, const T2& c2)
    : C1(c1)
    , C2(c2)
    {
    }

    bool operator () (const TMarkupItem& item) const {
        return C1(item) || C2(item);
    }
};

template <class T>
struct TLogicalNot {
    T C;

    TLogicalNot(const T& c)
    : C(c)
    {
    }

    bool operator () (const TMarkupItem& item) const {
        return !C(item);
    }
};

struct TAlwaysTrue {
    bool operator () (const TMarkupItem&) const {
        return true;
    }
};

template <class TMarkupTT, class TItem>
class TAllMarkupIteratorBase {
public:
    typedef TItem TValueType;
    typedef TMarkupTT TMarkupT;
private:
    TMarkupT* Markup;
    size_t ItemsArrayIndex;
    size_t ItemIndex;
public:
    TAllMarkupIteratorBase(TMarkupT& markup)
        : Markup(&markup)
        , ItemsArrayIndex(0)
        , ItemIndex(0)
    {
        FindItemsArray();
    }
    bool AtEnd() const {
        return IsLastArray() && NoMoreItems();
    }
    TAllMarkupIteratorBase<TMarkupT, TItem>& operator++() {
        if (!AtEnd()) {
            ++ItemIndex;
            FindItemsArray();
        }
        return *this;
    }
    TAllMarkupIteratorBase<TMarkupT, TItem>& Erase() {
        if (!AtEnd()) {
            TMarkup::TItems& items = Markup->ItemsArray[ItemsArrayIndex];
            items.erase(items.begin() + ItemIndex);
            FindItemsArray();
        }
        return *this;
    }
    TItem& operator*() const {
        return Markup->ItemsArray[ItemsArrayIndex][ItemIndex];
    }
    TItem* operator->() const {
        return &Markup->ItemsArray[ItemsArrayIndex][ItemIndex];
    }
private:
    void FindItemsArray() {
        while (!IsLastArray() && NoMoreItems()) {
            ItemIndex = 0;
            ++ItemsArrayIndex;
        }
    }
    bool NoMoreItems() const {
        return ItemIndex >= Markup->ItemsArray[ItemsArrayIndex].size();
    }
    bool IsLastArray() const {
        return ItemsArrayIndex == (Markup->ItemsArray.size() - 1);
    }
};

typedef TAllMarkupIteratorBase<TMarkup, TMarkupItem> TAllMarkupIterator;
typedef TAllMarkupIteratorBase<const TMarkup, const TMarkupItem> TAllMarkupConstIterator;

template <class TConcreteMarkupData, bool Const>
class TMarkupIteratorBase {
protected:
    typedef std::conditional_t<Const, const TMarkup::TItems, TMarkup::TItems> TMarkupItemsT;
    typedef std::conditional_t<Const, const TMarkup, TMarkup> TMarkupT;
    typedef std::conditional_t<Const, const TMarkupItem, TMarkupItem> TMarkupItemT;
public:
    typedef TMarkupItemT TValueType;
    typedef TConcreteMarkupData TDataType;
protected:
    TMarkupItemsT* Markup;
    size_t Current;
protected:
    TMarkupIteratorBase(typename TMarkupIteratorBase::TMarkupT& markup, size_t current)
        : Markup(&markup.template GetItems<TConcreteMarkupData>())
        , Current(current) {
    }
public:
    TConcreteMarkupData& GetData() const {
        return GetValue().template GetDataAs<TConcreteMarkupData>();
    }
    TMarkupItemT& operator*() const {
        return GetValue();
    }
    TMarkupItemT* operator->() const {
        return &GetValue();
    }
protected:
    size_t Index() const {
        return Current - 1;
    }
    TMarkupItemT& GetValue() const {
        return (*Markup)[Index()];
    }
    void DoErase() {
        Markup->erase(Markup->begin() + Index());
    }
};

template <class TConcreteMarkupData, bool Const>
class TForwardMarkupIterator: public TMarkupIteratorBase<TConcreteMarkupData, Const> {
public:
    typedef TMarkupIteratorBase<TConcreteMarkupData, Const> TBase;
    typedef typename TBase::TMarkupT TMarkupT;
public:
    TForwardMarkupIterator(TMarkupT& markup)
        : TBase(markup, 1) {
    }
    bool AtEnd() const {
        return TBase::Current > TBase::Markup->size();
    }
    TForwardMarkupIterator& operator++() {
        ++TBase::Current;
        return *this;
    }

    TForwardMarkupIterator& Erase() {
        if (!AtEnd())
            TBase::DoErase();
        return *this;
    }
};

template <class TConcreteMarkupData, bool Const>
class TBackwardMarkupIterator: public TMarkupIteratorBase<TConcreteMarkupData, Const> {
public:
    typedef TMarkupIteratorBase<TConcreteMarkupData, Const> TBase;
    typedef typename TBase::TMarkupT TMarkupT;
public:
    TBackwardMarkupIterator(typename TBase::TMarkupT& markup)
        : TBase(markup, markup.template GetItems<TConcreteMarkupData>().size()) {
    }
    bool AtEnd() const {
        return TBase::Current == 0;
    }
    TBackwardMarkupIterator& operator++() {
        --TBase::Current;
        return *this;
    }
    TBackwardMarkupIterator& Erase() {
        if (!AtEnd()) {
            TBase::DoErase();
            --TBase::Current;
        }
        return *this;
    }
};

template <class TBaseIterator, class TChecker>
class TGeneralCheckMarkupIterator {
public:
    typedef typename TBaseIterator::TValueType TValueType;
private:
    TBaseIterator Base;
    TChecker Checker;
public:
    TGeneralCheckMarkupIterator(const TBaseIterator& base, const TChecker& checker)
        : Base(base)
        , Checker(checker)
    {
        FindValid();
    }
    TGeneralCheckMarkupIterator& operator++() {
        if (!AtEnd()) {
            ++Base;
            FindValid();
        }
        return *this;
    }
    bool AtEnd() const {
        return Base.AtEnd();
    }
    TValueType& operator*() const {
        return GetValue();
    }
    TValueType* operator->() const {
        return &GetValue();
    }
protected:
    const TBaseIterator& GetBase() const {
        return Base;
    }
private:
    TValueType& GetValue() const {
        return *Base;
    }
    bool Check() {
        return Checker(GetValue());
    }
    void FindValid() {
        while (!AtEnd() && !Check())
            ++Base;
    }
};

template <class TBaseIterator, class TChecker>
class TCheckMarkupIterator: public TGeneralCheckMarkupIterator<TBaseIterator, TChecker> {
private:
    typedef TGeneralCheckMarkupIterator<TBaseIterator, TChecker> TIBase;
public:
    typedef typename TBaseIterator::TDataType TDataType;
public:
    TCheckMarkupIterator(const TBaseIterator& base, const TChecker& checker)
        : TIBase(base, checker)
    {
    }
    TDataType& GetData() const {
        return TIBase::GetBase().GetData();
    }
};

struct TCheckPosition {
    size_t Pos;

    TCheckPosition(size_t pos)
    : Pos(pos)
    {
    }

    bool operator () (const TMarkupItem& item) const {
        return RangeContains(item.Range, Pos);
    }
};

struct TCheckRangeFull {
    TRange Range;

    TCheckRangeFull(const TRange& range)
    : Range(range)
    {
    }

    bool operator () (const TMarkupItem& item) const {
        return item.Range.Contains(Range);
    }
};

struct TCheckRangePart {
    TRange Range;

    TCheckRangePart(const TRange& range)
    : Range(range)
    {
    }

    bool operator () (const TMarkupItem& item) const {
        return !!item.Range.Intersect(Range);
    }
};

template<class TConcreteMarkupData>
class TPosMarkupIterator: public TCheckMarkupIterator<TForwardMarkupIterator<TConcreteMarkupData, true>, TCheckPosition> {
public:
    typedef TForwardMarkupIterator<TConcreteMarkupData, true> TBaseBase;
    typedef TCheckMarkupIterator<TBaseBase, TCheckPosition> TBase;
    typedef typename TBaseBase::TMarkupT TMarkupT;
public:
    TPosMarkupIterator(TMarkupT& markup, size_t pos)
        : TBase(TBaseBase(markup), TCheckPosition(pos))
    {}
};

template<class TConcreteMarkupData>
class TPosAllMarkupIterator: public TGeneralCheckMarkupIterator<TAllMarkupConstIterator, TCheckPosition> {
public:
    typedef TAllMarkupConstIterator TBaseBase;
    typedef TGeneralCheckMarkupIterator<TBaseBase, TCheckPosition> TBase;
    typedef typename TBaseBase::TMarkupT TMarkupT;
public:
    TPosAllMarkupIterator(TMarkupT& markup, size_t pos)
        : TBase(TBaseBase(markup), TCheckPosition(pos))
    {}
};

} // namespace
