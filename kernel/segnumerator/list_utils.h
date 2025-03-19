#pragma once

#include <util/generic/intrlist.h>
#include <util/memory/pool.h>

namespace NSegm {
namespace NPrivate {

template<typename TIterator>
struct TReverseListIterator: public TIterator {
    explicit TReverseListIterator(TIterator it) :
        TIterator(it) {
    }

    TReverseListIterator& operator++() {
        TIterator::Prev();
        return *this;
    }

    TReverseListIterator operator++(int) {
        TReverseListIterator old = *this;
        TIterator::Prev();
        return old;
    }

    TReverseListIterator& operator--() {
        TIterator::Next();
        return *this;
    }

    TReverseListIterator operator--(int) {
        TReverseListIterator old = *this;
        TIterator::Next();
        return old;
    }
};


template<typename tnode, typename tlist = tnode>
struct TListAccessor {
    typedef tlist TListType;
    typedef tnode TNode;

    template<typename TIterator>
    static TNode * GetPrevPrev(TIterator it, TIterator begin) {
        if (it == begin || --it == begin)
            return nullptr;
        return &*--it;
    }

    template<typename TIterator>
    static TNode * GetPrev(TIterator it, TIterator begin) {
        return it == begin ? nullptr : &*--it;
    }

    template<typename TIterator>
    static TNode * GetNext(TIterator it, TIterator end) {
        return end == ++it ? nullptr : &*it;
    }

    template<typename TIterator>
    static TNode * GetNextNext(TIterator it, TIterator end) {
        if (++it == end || ++it == end)
            return nullptr;
        return &*it;
    }

    static TNode * Front(TListType& segs) {
        return &*(segs.Begin());
    }

    static TNode * Back(TListType& segs) {
        return &*(--segs.End());
    }

    static bool Empty(TListType& segs) {
        return segs.Empty();
    }

    static typename TListType::iterator Begin(TListType& segs) {
        return segs.Begin();
    }

    static typename TListType::iterator End(TListType& segs) {
        return segs.End();
    }

    static typename TListType::iterator Erase(typename TListType::iterator it, const TListType*l = nullptr) {
        (void) l;
        typename TListType::iterator tit = it;
        ++it;
        tit->Unlink();
        return it;
    }

    static typename TListType::iterator EraseBack(typename TListType::iterator it, const TListType*l = nullptr) {
        (void) l;
        typename TListType::iterator tit = it;
        --it;
        tit->Unlink();
        return it;
    }
};

}
}
