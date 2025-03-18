#pragma once

#include "intrhash.h"
#include "nodeallc.h"

template <class T, class O, class A>
struct THashSetPrivate {
    using TValue = T;

    struct TNode: public TIntrusiveHashItem<TNode> {
        inline TNode(const TValue& value)
            : Value(value)
        {
        }

        const TValue Value;
    };

    struct TOps: public O {
        static inline const TValue& ExtractKey(const TNode& node) noexcept {
            return node.Value;
        }

        static inline const TValue& ExtractValue(TNode& node) noexcept {
            return node.Value;
        }

        static inline const TValue& ExtractValue(const TNode& node) noexcept {
            return node.Value;
        }
    };

    using TAllc = TNodeAllocator<TNode, A>;
    using TImpl = TIntrusiveHash<TNode, TOps, A>;
};

template <class T, class O = TCommonIntrHashOps, class A = std::allocator<T>>
class THashSetType: private THashSetPrivate<T, O, A>::TAllc, private THashSetPrivate<T, O, A>::TImpl {
private:
    using TPrivate = THashSetPrivate<T, O, A>;

    using TOps = typename TPrivate::TOps;
    using TAllc = typename TPrivate::TAllc;
    using TImpl = typename TPrivate::TImpl;

    using TItem = typename TImpl::TItem;
    using TNode = typename TImpl::TNode;

public:
    using TValue = typename TPrivate::TValue;
    using value_type = TValue;

public:
    using TIterator = typename TImpl::TIterator;
    using TConstIterator = typename TImpl::TConstIterator;

    using iterator = typename TImpl::iterator;
    using const_iterator = typename TImpl::const_iterator;

public:
    using TAllc::GetAllocator;

    using TImpl::Begin;
    using TImpl::CBegin;
    using TImpl::CEnd;
    using TImpl::End;
    using TImpl::begin;
    using TImpl::cbegin;
    using TImpl::cend;
    using TImpl::end;

    using TImpl::Count;
    using TImpl::EqualRange;
    using TImpl::Find;
    using TImpl::Has;
    using TImpl::count;
    using TImpl::equal_range;
    using TImpl::find;
    using TImpl::has;

    using TImpl::Empty;
    using TImpl::Size;
    using TImpl::empty;
    using TImpl::size;

public:
    inline std::pair<TIterator, bool> Insert(const TValue& value) {
        return this->FindOrPush(value, [this, &value]() { return this->NewNode(value); });
    }

    inline size_t Erase(TIterator i) {
        DeleteNode(this->Pop(i));
        return 1;
    }

    template <class TKey>
    inline size_t Erase(const TKey& key) {
        TNode* const node = this->Pop(key);

        if (node) {
            this->DeleteNode(node);
            return 1;
        }

        return 0;
    }

    inline void Clear() {
        this->Decompose([this](TNode* node) { this->DeleteNode(node); });
    }

public:
    inline std::pair<iterator, bool> insert(const TValue& value) {
        return Insert(value);
    }

    inline size_t erase(iterator i) {
        return Erase(i);
    }

    template <class TKey>
    inline size_t erase(const TKey& key) {
        return Erase(key);
    }

    inline void clear() {
        return Clear();
    }

public:
    inline THashSetType() = default;

    inline THashSetType(size_t n)
        : TImpl(n)
    {
    }

    template <class TAllocParam>
    inline THashSetType(TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(allocParam)
    {
    }

    template <class TAllocParam>
    inline THashSetType(size_t n, TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(n, allocParam)
    {
    }

    inline THashSetType(THashSetType&& right)
        : TAllc(std::move(right))
        , TImpl(std::move(right))
    {
    }

    inline THashSetType(const THashSetType& right)
        : TAllc(right)
        , TImpl(right, [this](const TNode* node) { return this->NewNode(*node); })
    {
    }

    inline ~THashSetType() noexcept {
        Clear();
    }

    inline void Swap(THashSetType& right) noexcept {
        DoSwap<TAllc>(*this, right);
        DoSwap<TImpl>(*this, right);
    }

    inline void swap(THashSetType& right) noexcept {
        return Swap(right);
    }

    inline THashSetType& operator=(const THashSetType& right) {
        THashSetType(right).Swap(*this);
        return *this;
    }

    inline THashSetType& operator=(THashSetType&& right) {
        THashSetType(std::move(right)).Swap(*this);
        return *this;
    }
};

template <class T, class O = TCommonIntrHashOps, class A = std::allocator<T>>
class THashMultiSetType: private THashSetPrivate<T, O, A>::TAllc, private THashSetPrivate<T, O, A>::TImpl {
private:
    using TPrivate = THashSetPrivate<T, O, A>;

    using TOps = typename TPrivate::TOps;
    using TAllc = typename TPrivate::TAllc;
    using TImpl = typename TPrivate::TImpl;

    using TItem = typename TImpl::TItem;
    using TNode = typename TImpl::TNode;

public:
    using TValue = typename TPrivate::TValue;
    using value_type = TValue;

public:
    using TIterator = typename TImpl::TIterator;
    using TConstIterator = typename TImpl::TConstIterator;

    using iterator = typename TImpl::iterator;
    using const_iterator = typename TImpl::const_iterator;

public:
    using TAllc::GetAllocator;

    using TImpl::Begin;
    using TImpl::CBegin;
    using TImpl::CEnd;
    using TImpl::End;
    using TImpl::begin;
    using TImpl::cbegin;
    using TImpl::cend;
    using TImpl::end;

    using TImpl::Count;
    using TImpl::EqualRange;
    using TImpl::Find;
    using TImpl::Has;
    using TImpl::count;
    using TImpl::equal_range;
    using TImpl::find;
    using TImpl::has;

    using TImpl::Empty;
    using TImpl::Size;
    using TImpl::empty;
    using TImpl::size;

public:
    inline TIterator Insert(const TValue& value) {
        return this->Push(this->NewNode(value));
    }

    inline size_t Erase(TIterator i) {
        DeleteNode(this->Pop(i));
        return 1;
    }

    template <class TKey>
    inline size_t Erase(const TKey& key) {
        size_t ret = 0;
        this->PopAll(key, [this, &ret](TNode* node) { this->DeleteNode(node); ++ret; });
        return ret;
    }

    inline void Clear() {
        this->Decompose([this](TNode* node) { this->DeleteNode(node); });
    }

public:
    inline iterator insert(const TValue& value) {
        return Insert(value);
    }

    inline size_t erase(iterator i) {
        return Erase(i);
    }

    template <class TKey>
    inline size_t erase(const TKey& key) {
        return Erase(key);
    }

    inline void clear() {
        return Clear();
    }

public:
    inline THashMultiSetType() = default;

    inline THashMultiSetType(size_t n)
        : TImpl(n)
    {
    }

    template <class TAllocParam>
    inline THashMultiSetType(TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(allocParam)
    {
    }

    template <class TAllocParam>
    inline THashMultiSetType(size_t n, TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(n, allocParam)
    {
    }

    inline THashMultiSetType(THashMultiSetType&& right) noexcept
        : TAllc(std::move(right))
        , TImpl(std::move(right))
    {
    }

    inline THashMultiSetType(const THashMultiSetType& right)
        : TAllc(right)
        , TImpl(right, [this](const TNode* node) { return this->NewNode(*node); })
    {
    }

    inline ~THashMultiSetType() noexcept {
        Clear();
    }

    inline void Swap(THashMultiSetType& right) noexcept {
        DoSwap<TAllc>(*this, right);
        DoSwap<TImpl>(*this, right);
    }

    inline void swap(THashMultiSetType& right) noexcept {
        return Swap(right);
    }

    inline THashMultiSetType& operator=(const THashMultiSetType& right) {
        THashMultiSetType(right).Swap(*this);
        return *this;
    }

    inline THashMultiSetType& operator=(THashMultiSetType&& right) noexcept {
        THashMultiSetType(std::move(right)).Swap(*this);
        return *this;
    }
};
