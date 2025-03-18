#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/hash_primes.h>
#include <utility>
#include <util/generic/singleton.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/str_stl.h>

template <class T>
class TIntrusiveHashItem {
public:
    using TNode = T;
    using TItem = TIntrusiveHashItem<T>;

    inline TNode* Node() noexcept {
        return static_cast<TNode*>(this);
    }

    inline const TNode* Node() const noexcept {
        return static_cast<const TNode*>(this);
    }

    inline bool Linked() const noexcept {
        return Next_;
    }

public:
    inline TItem* Next() noexcept {
        return Next_;
    }

    inline const TItem* Next() const noexcept {
        return Next_;
    }

    inline TItem** NextPtr() noexcept {
        return &Next_;
    }

    inline const TItem* const* NextPtr() const noexcept {
        return &Next_;
    }

    inline void SetNext(TItem* next) noexcept {
        Next_ = next;
    }

private:
    TItem* Next_ = nullptr;
};

struct TCommonIntrHashOps {
    template <class T>
    static inline size_t Hash(const T& k) {
        return Default<THash<T>>()(k);
    }

    template <class TF, class TS>
    static inline bool EqualTo(const TF& f, const TS& s) {
        return f == s;
    }

    template <class T>
    static inline T& ExtractValue(T& t) noexcept {
        return t;
    }
};

template <class T, class TOps, class TAllocator = std::allocator<T>>
class TIntrusiveHash {
protected:
    using TItem = TIntrusiveHashItem<T>;
    using TNode = typename TItem::TNode;

private:
    template <bool Constant>
    class TContextBase {
    public:
        using TItem = std::conditional_t<Constant, const typename TIntrusiveHash::TItem, typename TIntrusiveHash::TItem>;
        using TNode = std::conditional_t<Constant, const typename TIntrusiveHash::TNode, typename TIntrusiveHash::TNode>;

        using TItemPtr = std::conditional_t<Constant, TItem* const, TItem*>;

        template <bool Constant_>
        inline TContextBase(const TContextBase<Constant_>& right) noexcept
            : Ptr_(right.Ptr())
        {
        }

        inline TContextBase(TItemPtr* ptr) noexcept
            : Ptr_(ptr)
        {
        }

        inline TItemPtr* Ptr() const noexcept {
            return Ptr_;
        }

        inline TItem* Item() const noexcept {
            return *Ptr();
        }

        inline TNode* Node() const noexcept {
            return Item()->Node();
        }

    private:
        TItemPtr* Ptr_ = nullptr;
    };

    using TContext = TContextBase<false>;
    using TConstContext = TContextBase<true>;

    template <class TCtx>
    class TFoundContext: public TCtx {
    public:
        inline TFoundContext(TCtx ctx, bool found)
            : TCtx(ctx)
            , Found_(found)
        {
        }

        inline TCtx Ctx() const noexcept {
            return *this;
        }

        inline bool Found() const noexcept {
            return Found_;
        }

    private:
        const bool Found_;
    };

private:
    template <class TCtx>
    static inline bool CtxIsBucket(TCtx ctx) noexcept {
        return reinterpret_cast<uintptr_t>(*ctx.Ptr()) & 1;
    }

    template <class TCtx>
    static inline TCtx CtxNextItem(TCtx ctx) noexcept {
        return {ctx.Item()->NextPtr()};
    }

    template <class TCtx>
    static inline TCtx CtxNextBucket(TCtx ctx) noexcept {
        return {reinterpret_cast<typename TCtx::TItemPtr*>(reinterpret_cast<uintptr_t>(*ctx.Ptr()) & ~1)};
    }

    template <class TCtx, class TKey>
    static inline bool CtxRelative(TCtx ctx, const TKey& key) noexcept {
        return TOps::EqualTo(TOps::ExtractKey(*ctx.Node()), key);
    }

    static inline void CtxPush(TContext ctx, TItem* item) noexcept {
        item->SetNext(ctx.Item());
        *ctx.Ptr() = item;
    }

    static inline TItem* CtxPop(TContext ctx) noexcept {
        TItem* const item = ctx.Item();
        *ctx.Ptr() = item->Next();
        item->SetNext(nullptr);
        return item;
    }

private:
    using TBuckets = TVector<TItem*, TAllocator>;

private:
    template <class TCtx, class TBkts, class TKey>
    static inline TCtx BaseContext(TBkts& bkts, const TKey& key) noexcept {
        return &bkts[(TOps::Hash(key) % (bkts.size() - 1))];
    }

    template <class TCtx>
    static inline TCtx ItemContext(TCtx ctx) noexcept {
        while (CtxIsBucket(ctx)) {
            ctx = CtxNextBucket(ctx);
        }
        return ctx;
    }

    template <class TCtx, class TBkts, class TKey>
    static inline TFoundContext<TCtx> FindContextImpl(TBkts& bkts, const TKey& key) noexcept {
        TCtx ctx = BaseContext<TCtx>(bkts, key);

        while (!CtxIsBucket(ctx)) {
            if (CtxRelative(ctx, key)) {
                return {ctx, true};
            }
            ctx = CtxNextItem(ctx);
        }

        return {ctx, false};
    }

    template <class TKey>
    inline TFoundContext<TContext> FindContext(const TKey& key) noexcept {
        return FindContextImpl<TContext>(Buckets_, key);
    }

    template <class TKey>
    inline TFoundContext<TConstContext> FindContext(const TKey& key) const noexcept {
        return FindContextImpl<TConstContext>(Buckets_, key);
    }

    static inline void InitBuckets(TBuckets* buckets) noexcept {
        if (!buckets->empty()) {
            typename TBuckets::iterator bucket = buckets->begin();
            typename TBuckets::iterator last = buckets->end();

            --last;

            while (bucket != last) {
                const typename TBuckets::iterator current = bucket;
                *current = reinterpret_cast<TItem*>(reinterpret_cast<uintptr_t>(&*++bucket) | 1);
            }

            *bucket = nullptr;
        }
    }

    template <class I, class TThs, class TKey>
    static inline std::pair<I, I> EqualRangeImpl(TThs* ths, const TKey& key) noexcept {
        const auto& base = ths->FindContext(key);

        if (!base.Found()) {
            return {ths->End(), ths->End()};
        }

        auto ctx = base.Ctx();

        do {
            ctx = CtxNextItem(ctx);
        } while (!CtxIsBucket(ctx) && CtxRelative(ctx, key));

        return {base, ItemContext(ctx)};
    }

private:
    template <bool Constant>
    class TIteratorBase {
    private:
        using TCtx = TContextBase<Constant>;

    public:
        using TItem = typename TCtx::TItem;
        using TNode = typename TCtx::TNode;

        using TValue = std::remove_reference_t<decltype(TOps::ExtractValue(*reinterpret_cast<TNode*>(0)))>;

    public:
        using value_type = TValue;
        using reference = TValue&;
        using pointer = TValue*;

        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;

    public:
        inline TIteratorBase() = default;

        template <bool Constant_>
        inline TIteratorBase(const TIteratorBase<Constant_>& right) noexcept
            : Ctx_(right.Ctx())
        {
        }

        inline TIteratorBase(TCtx ctx) noexcept
            : Ctx_(std::move(ctx))
        {
        }

        inline TIteratorBase(const TIteratorBase&) = default;

        inline TItem* Item() const noexcept {
            return Ctx_.Item();
        }

        inline TNode* Node() const noexcept {
            return Ctx_.Node();
        }

    public:
        const TCtx Ctx() const noexcept {
            return Ctx_;
        }

        inline void Next() noexcept {
            Ctx_ = ItemContext(CtxNextItem(Ctx_));
        }

    public:
        inline TIteratorBase& operator=(const TIteratorBase&) = default;

        inline TValue* operator->() const noexcept {
            return &TOps::ExtractValue(*Node());
        }

        inline TValue& operator*() const noexcept {
            return TOps::ExtractValue(*Node());
        }

        template <bool Constant_>
        inline bool operator==(const TIteratorBase<Constant_>& right) const noexcept {
            return Ctx().Ptr() == right.Ctx().Ptr();
        }

        template <bool Constant_>
        inline bool operator!=(const TIteratorBase<Constant_>& right) const noexcept {
            return !(*this == right);
        }

        inline TIteratorBase operator++() noexcept {
            Next();
            return *this;
        }

        inline TIteratorBase operator++(int) noexcept {
            const TIteratorBase ret(*this);
            Next();
            return ret;
        }

    private:
        TCtx Ctx_;
    };

public:
    using TIterator = TIteratorBase<false>;
    using TConstIterator = TIteratorBase<true>;

    using iterator = TIterator;
    using const_iterator = TConstIterator;

public:
    inline TIterator Begin() noexcept {
        return {ItemContext<TContext>(&*Buckets_.begin())};
    }

    inline TIterator End() noexcept {
        return {&*Buckets_.rbegin()};
    }

    inline TConstIterator Begin() const noexcept {
        return {ItemContext<TConstContext>(&*Buckets_.begin())};
    }

    inline TConstIterator End() const noexcept {
        return {&*Buckets_.rbegin()};
    }

    inline TConstIterator CBegin() const noexcept {
        return Begin();
    }

    inline TConstIterator CEnd() const noexcept {
        return End();
    }

public:
    inline iterator begin() noexcept {
        return Begin();
    }

    inline iterator end() noexcept {
        return End();
    }

    inline const_iterator begin() const noexcept {
        return Begin();
    }

    inline const_iterator end() const noexcept {
        return End();
    }

    inline const_iterator cbegin() const noexcept {
        return CBegin();
    }

    inline const_iterator cend() const noexcept {
        return CEnd();
    }

public:
    template <class TCbk>
    inline void Decompose(TCbk cbk) {
        for (TContext ctx = &*Buckets_.begin(); ctx.Ptr() != &*Buckets_.rbegin();) {
            if (!CtxIsBucket(ctx)) {
                --Items_;
                cbk(CtxPop(ctx)->Node());
            } else {
                ctx = CtxNextBucket(ctx);
            }
        }
    }

    inline void Decompose() noexcept {
        Decompose([](TNode*) {});
    }

    inline void Resize(size_t n) {
        if (n > Buckets_.size() - 1) {
            const size_t nbuckets = HashBucketCount(n) + 1;

            if (nbuckets > Buckets_.size()) {
                TBuckets buckets(nbuckets, nullptr, Buckets_.get_allocator());

                InitBuckets(&buckets);
                Decompose([this, &buckets](TItem* item) { CtxPush(BaseContext<TContext>(buckets, TOps::ExtractKey(*item->Node())), item); ++Items_; });
                DoSwap(Buckets_, buckets);
            }
        }
    }

public:
    template <class TKey>
    inline TIterator Find(const TKey& key) noexcept {
        const TFoundContext<TContext> ctx = FindContext(key);
        return ctx.Found() ? TIterator(ctx) : End();
    }

    template <class TKey>
    inline TConstIterator Find(const TKey& key) const noexcept {
        const TFoundContext<TConstContext> ctx = FindContext(key);
        return ctx.Found() ? TConstIterator(ctx) : End();
    }

    template <class TKey>
    inline TNode* FindPtr(const TKey& key) noexcept {
        const TFoundContext<TContext> ctx = FindContext(key);
        return ctx.Found() ? ctx.Node() : nullptr;
    }

    template <class TKey>
    inline const TNode* FindPtr(const TKey& key) const noexcept {
        const TFoundContext<TConstContext> ctx = FindContext(key);
        return ctx.Found() ? ctx.Node() : nullptr;
    }

    template <class TKey>
    inline bool Has(const TKey& key) const noexcept {
        return Find(key) != End();
    }

    template <class TKey>
    inline std::pair<TIterator, TIterator> EqualRange(const TKey& key) noexcept {
        return EqualRangeImpl<TIterator>(this, key);
    }

    template <class TKey>
    inline std::pair<TConstIterator, TConstIterator> EqualRange(const TKey& key) const noexcept {
        return EqualRangeImpl<TConstIterator>(this, key);
    }

    template <class TKey>
    inline size_t Count(const TKey& key) const noexcept {
        const TFoundContext<TConstContext> base = FindContext(key);

        if (!base.Found()) {
            return 0;
        }

        size_t ret = 0;
        TConstContext ctx = base;

        do {
            ++ret;
            ctx = CtxNextItem(ctx);
        } while (!CtxIsBucket(ctx) && CtxRelative(ctx, key));

        return ret;
    }

public:
    template <class TKey>
    inline iterator find(const TKey& key) noexcept {
        return Find(key);
    }

    template <class TKey>
    inline const_iterator find(const TKey& key) const noexcept {
        return Find(key);
    }

    template <class TKey>
    inline bool has(const TKey& key) const noexcept {
        return Has(key);
    }

    template <class TKey>
    inline std::pair<iterator, iterator> equal_range(const TKey& key) noexcept {
        return EqualRange(key);
    }

    template <class TKey>
    inline std::pair<const_iterator, const_iterator> equal_range(const TKey& key) const noexcept {
        return EqualRange(key);
    }

    template <class TKey>
    inline size_t count(const TKey& key) const noexcept {
        return Count(key);
    }

public:
    inline size_t Size() const noexcept {
        return Items_;
    }

    inline bool Empty() const noexcept {
        return !Size();
    }

public:
    inline size_t size() const noexcept {
        return Size();
    }

    inline bool empty() const noexcept {
        return Empty();
    }

public:
    inline TIterator PushNoResize(TNode* node) noexcept {
        const TContext ctx = FindContext(TOps::ExtractKey(*node));
        ++Items_;
        CtxPush(ctx, node);
        return {ctx};
    }

    inline TIterator Push(TNode* node) noexcept {
        Resize(Items_ + 1);
        return PushNoResize(node);
    }

    inline TNode* Pop(TIterator i) noexcept {
        return CtxPop(i.Ctx())->Node();
    }

    template <class TKey>
    inline TNode* Pop(const TKey& key) noexcept {
        const TFoundContext<TContext> ctx = FindContext(key);

        if (ctx.Found()) {
            --Items_;
            return CtxPop(ctx)->Node();
        } else {
            return nullptr;
        }
    }

    template <class TKey, class TCbk>
    inline void PopAll(const TKey& key, TCbk cbk) {
        const TFoundContext<TContext> ctx = FindContext(key);

        if (ctx.Found()) {
            do {
                --Items_;
                cbk(CtxPop(ctx)->Node());
            } while (!CtxIsBucket(ctx) && CtxRelative(ctx, key));
        }
    }

    template <class TKey>
    inline void PopAll(const TKey& key) noexcept {
        PopAll(key, [](TNode*) {});
    }

    template <class TKey, class TGen>
    inline std::pair<TIterator, bool> FindOrPushNoResize(const TKey& key, const TGen& gen) {
        const TFoundContext<TContext> ctx = FindContext(key);

        if (!ctx.Found()) {
            ++Items_;
            CtxPush(ctx, gen());
            return {ctx, true};
        }

        return {ctx, false};
    }

    template <class TKey, class TGen>
    inline std::pair<TIterator, bool> FindOrPush(const TKey& key, TGen gen) {
        Resize(Items_ + 1);
        return FindOrPushNoResize(key, std::move(gen));
    }

private:
    TIntrusiveHash(const TIntrusiveHash&) = delete;
    TIntrusiveHash& operator=(const TIntrusiveHash&) = delete;

public:
    inline TIntrusiveHash()
        : TIntrusiveHash(0)
    {
    }

    inline TIntrusiveHash(size_t n)
        : Buckets_(HashBucketCount(n) + 1)
    {
        InitBuckets(&Buckets_);
    }

    template <class TAllocParam>
    inline TIntrusiveHash(TAllocParam* allocParam)
        : TIntrusiveHash(0, allocParam)
    {
    }

    template <class TAllocParam>
    explicit inline TIntrusiveHash(size_t n, TAllocParam* allocParam)
        : Buckets_(HashBucketCount(n) + 1, 0, allocParam)
    {
        InitBuckets(&Buckets_);
    }

    inline TIntrusiveHash(TIntrusiveHash&& right)
        : Buckets_(std::move(right.Buckets_))
        , Items_(std::move(right.Items_))
    {
        right.Buckets_.resize(2);
        InitBuckets(&right.Buckets_);
        right.Items_ = 0;
    }

    inline ~TIntrusiveHash() noexcept {
        Decompose();
    }

    inline void Swap(TIntrusiveHash& right) noexcept {
        DoSwap(Buckets_, right.Buckets_);
        DoSwap(Items_, right.Items_);
    }

protected:
    template <class TGen>
    inline TIntrusiveHash(const TIntrusiveHash& right, TGen gen)
        : Buckets_(right.Buckets_.size(), nullptr, right.Buckets_.get_allocator())
        , Items_(right.Items_)
    {
        InitBuckets(&Buckets_);

        for (size_t i = 0; i != Buckets_.size() - 1; ++i) {
            TConstContext ctx(&right.Buckets_[i]);
            TContext ins(&Buckets_[i]);

            while (!CtxIsBucket(ctx)) {
                CtxPush(ins, gen(ctx.Node()));
                ctx = CtxNextItem(ctx);
            }
        }
    }

private:
    TBuckets Buckets_;
    size_t Items_ = 0;
};

template <class T, class TOps, class D = TDelete, class TAllocator = std::allocator<T>>
class TIntrusiveHashWithAutoDelete: public TIntrusiveHash<T, TOps, TAllocator> {
private:
    using TBase = TIntrusiveHash<T, TOps, TAllocator>;

    using TItem = typename TBase::TItem;
    using TNode = typename TBase::TNode;

public:
    using TIterator = typename TBase::TIterator;
    using TConstIterator = typename TBase::TConstIterator;

    using iterator = typename TBase::iterator;
    using const_iterator = typename TBase::const_iterator;

public:
    inline TIntrusiveHashWithAutoDelete() = default;

    inline TIntrusiveHashWithAutoDelete(size_t n)
        : TBase(n)
    {
    }

    template <class TAllocParam>
    inline TIntrusiveHashWithAutoDelete(size_t n, TAllocParam* allocParam)
        : TBase(n, allocParam)
    {
    }

    inline TIntrusiveHashWithAutoDelete(TIntrusiveHashWithAutoDelete&& right) noexcept
        : TBase(std::move(right))
    {
    }

    inline ~TIntrusiveHashWithAutoDelete() noexcept {
        Clear();
    }

public:
    inline void Erase(TIterator i) noexcept {
        D::Destroy(Pop(i));
    }

    template <class TKey>
    inline size_t Erase(const TKey& key) noexcept {
        TNode* const node = this->Pop(key);

        if (node) {
            D::Destroy(node);
            return 1;
        }

        return 0;
    }

    template <class TKey>
    inline size_t EraseAll(const TKey& key) noexcept {
        size_t ret = 0;
        this->PopAll(key, [&ret](TNode* node) { D::Destroy(node); ++ret; });
        return ret;
    }

    inline void Clear() noexcept {
        this->Decompose([](TNode* node) { D::Destroy(node); });
    }
};
