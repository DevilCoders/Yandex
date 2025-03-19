#pragma once

namespace NSegm {


template<typename tlist, typename tnode>
struct TAccessor {
    typedef tlist TListType;
    typedef tnode TNode;
typedef    typename tlist::iterator TIterator;
    typedef typename tlist::const_iterator TConstIterator;

    static TNode * GetPrev(TIterator it, TIterator begin) {
        return it == begin ? nullptr : &*--it;
    }

    static const TNode * GetPrev(TConstIterator it, TConstIterator begin) {
        return it == begin ? 0 : &*--it;
    }

    static TNode * GetNext(TIterator it, TIterator end) {
        return end == ++it ? nullptr : &*it;
    }

    static const TNode * GetNext(TConstIterator it, TConstIterator end) {
        return end == ++it ? 0 : &*it;
    }

    static TNode * Front(TListType& segs) {
        return segs.empty() ? nullptr : &segs.front();
    }

    static const TNode * Front(const TListType& segs) {
        return segs.empty() ? 0 : &segs.front();
    }

    static TNode * Back(TListType& segs) {
        return segs.empty() ? nullptr : &segs.back();
    }

    static const TNode * Back(const TListType& segs) {
        return segs.empty() ? 0 : &segs.back();
    }

    static bool Empty(const TListType& segs) {
        return segs.empty();
    }

    static TIterator Begin(TListType& segs) {
        return segs.begin();
    }

    static TConstIterator Begin(const TListType& segs) {
        return segs.begin();
    }

    static TIterator End(TListType& segs) {
        return segs.end();
    }

    static TConstIterator End(const TListType& segs) {
        return segs.end();
    }

    static TIterator Erase(TIterator it, TListType*l) {
        return l->erase(it);
    }

    static TIterator EraseBack(TIterator it, TListType*l=0) {
        it = l->erase(it);
        return --it;
    }
};

}
