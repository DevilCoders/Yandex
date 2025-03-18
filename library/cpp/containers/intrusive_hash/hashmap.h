#pragma once

#include "intrhash.h"
#include "nodeallc.h"

template <class K, class T, class O, class A>
struct THashMapPrivate {
    using TValue = std::pair<const K, T>;

    struct TNode: public TValue, public TIntrusiveHashItem<TNode> {
        inline TNode(const TValue& value)
            : TValue(value)
        {
        }

        template <class TKey>
        inline TNode(const TKey& key)
            : TValue(key, T())
        {
        }
    };

    struct TOps: public O {
        static inline const K& ExtractKey(const TValue& node) noexcept {
            return node.first;
        }

        static inline TValue& ExtractValue(TNode& node) noexcept {
            return node;
        }

        static inline const TValue& ExtractValue(const TNode& node) noexcept {
            return node;
        }
    };

    using TAllc = TNodeAllocator<TNode, A>;
    using TImpl = TIntrusiveHash<TNode, TOps, A>;
};

template <class K, class T, class O = TCommonIntrHashOps, class A = std::allocator<T>>
class THashMapType: private THashMapPrivate<K, T, O, A>::TAllc, private THashMapPrivate<K, T, O, A>::TImpl {
private:
    using TPrivate = THashMapPrivate<K, T, O, A>;

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
        return this->FindOrPush(TOps::ExtractKey(value), [this, &value]() { return this->NewNode(value); });
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
    template <class TKey>
    inline T& At(const TKey& key) {
        const TIterator i = Find(key);

        if (i == End()) {
            throw std::out_of_range("THashMapType::At");
        }

        return i->second;
    }

    template <class TKey>
    inline T& At(const TKey& key) const {
        const TConstIterator i = Find(key);

        if (i == End()) {
            throw std::out_of_range("THashMapType::At");
        }

        return i->second;
    }

public:
    template <class TKey>
    inline T& at(const TKey& key) {
        return At(key);
    }

    template <class TKey>
    inline T& at(const TKey& key) const {
        return At(key);
    }

public:
    template <class TKey>
    inline T& operator[](const TKey& key) {
        return this->FindOrPush(key, [this, &key]() { return this->NewNode(key); }).first->second;
    }

public:
    inline THashMapType() = default;

    inline THashMapType(size_t n)
        : TImpl(n)
    {
    }

    template <class TAllocParam>
    inline THashMapType(TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(allocParam)
    {
    }

    template <class TAllocParam>
    inline THashMapType(size_t n, TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(n, allocParam)
    {
    }

    inline THashMapType(THashMapType&& right)
        : TAllc(std::move(right))
        , TImpl(std::move(right))
    {
    }

    inline THashMapType(const THashMapType& right)
        : TAllc(right)
        , TImpl(right, [this](const TNode* node) { return this->NewNode(*node); })
    {
    }

    inline ~THashMapType() noexcept {
        Clear();
    }

    inline void Swap(THashMapType& right) noexcept {
        DoSwap<TAllc>(*this, right);
        DoSwap<TImpl>(*this, right);
    }

    inline void swap(THashMapType& right) noexcept {
        return Swap(right);
    }

    inline THashMapType& operator=(const THashMapType& right) {
        THashMapType(right).Swap(*this);
        return *this;
    }

    inline THashMapType& operator=(THashMapType&& right) {
        THashMapType(std::move(right)).Swap(*this);
        return *this;
    }
};

template <class K, class T, class O = TCommonIntrHashOps, class A = std::allocator<T>>
class THashMultiMapType: private THashMapPrivate<K, T, O, A>::TAllc, private THashMapPrivate<K, T, O, A>::TImpl {
private:
    using TPrivate = THashMapPrivate<K, T, O, A>;

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
    inline THashMultiMapType() = default;

    inline THashMultiMapType(size_t n)
        : TImpl(n)
    {
    }

    template <class TAllocParam>
    inline THashMultiMapType(TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(allocParam)
    {
    }

    template <class TAllocParam>
    inline THashMultiMapType(size_t n, TAllocParam* allocParam)
        : TAllc(allocParam)
        , TImpl(n, allocParam)
    {
    }

    inline THashMultiMapType(THashMultiMapType&& right) noexcept
        : TAllc(std::move(right))
        , TImpl(std::move(right))
    {
    }

    inline THashMultiMapType(const THashMultiMapType& right)
        : TAllc(right)
        , TImpl(right, [this](const TNode* node) { return this->NewNode(*node); })
    {
    }

    inline ~THashMultiMapType() noexcept {
        Clear();
    }

    inline void Swap(THashMultiMapType& right) noexcept {
        DoSwap<TAllc>(*this, right);
        DoSwap<TImpl>(*this, right);
    }

    inline void swap(THashMultiMapType& right) noexcept {
        return Swap(right);
    }

    inline THashMultiMapType& operator=(const THashMultiMapType& right) {
        THashMultiMapType(right).Swap(*this);
        return *this;
    }

    inline THashMultiMapType& operator=(THashMultiMapType&& right) noexcept {
        THashMultiMapType(std::move(right)).Swap(*this);
        return *this;
    }
};
