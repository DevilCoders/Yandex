#pragma once

namespace NIndexerCore {
namespace NIndexerCorePrivate {

template <class TItem>
struct TListType {
    struct TNode {
        TNode* Next;
        TItem Item;
    };

    struct TConstIterator {
        TNode* Current;
        void operator ++ () {
            Current = Current->Next;
        }
        const TItem* operator->() const {
            return &Current->Item;
        }
        const TItem& operator *() const {
            return Current->Item;
        }
        bool operator !=(const TConstIterator& it) const {
            return Current != it.Current;
        }
        bool operator ==(const TConstIterator& it) const {
            return Current == it.Current;
        }
        bool Valid() const {
            return Current != nullptr;
        }
    };

    TNode* Head;
    TNode* Tail;

    TListType()
        : Head (nullptr)
        , Tail (nullptr)
    {
    }
    void Clear() {
        Head = 0;
        Tail = 0;
    }
    void PushBack(TNode* t) {
        if (Head == nullptr)
            Tail = Head = t;
        else
            Tail = Tail->Next = t;
        t->Next = nullptr;
    }
    bool Empty() const {
        return Head == nullptr;
    }
    TConstIterator Begin() const {
        TConstIterator it = {Head};
        return it;
    }
    TConstIterator End() const {
        TConstIterator it = {nullptr};
        return it;
    }
    TItem& Back() {
        return Tail->Item;
    }
};

// for MultHitHeap
template <class TItem>
class TRestartableIlistIterator : public TListType<TItem>::TConstIterator {
private:
    typedef typename TListType<TItem>::TConstIterator TIteratorBase;
public:
    using TIteratorBase::Current;
    typedef TRestartableIlistIterator<TItem>* ptr;
    typedef TItem value_type;

public:
    void Restart() {
        Current = Container->Head;
    }
    void SetContainer(const TListType<TItem>* c) {
        Container = c;
    }
    ptr TopIter() {
       return this;
    }
private:
    const TListType<TItem>* Container;
};

}}
