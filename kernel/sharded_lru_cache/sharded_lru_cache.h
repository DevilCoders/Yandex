#pragma once

#include <kernel/sharded_lru_cache/proto/sharded_lru_cache_config.pb.h>

#include <library/cpp/cache/cache.h>

#include <util/datetime/base.h>
#include <util/digest/numeric.h>
#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/system/mutex.h>
#include <util/digest/multi.h>
#include <util/digest/murmur.h>
#include <util/random/shuffle.h>

#include <array>
#include <type_traits>

namespace NShardedCache {

    namespace NDetail {
        template <typename TKey, typename TValue, bool Degrade = false, typename TSizeProvider = TUniformSizeProvider<TValue>, typename TAllocator = std::allocator<void>>
        class TTimedLruCache {
            struct TCacheItem {
                TValue Value;
                TInstant Deadline; // use a monotonic clock?

                template<typename TValueRef>
                TCacheItem(TValueRef&& value, TInstant deadline, TInstant hardDeadline)
                    : Value(std::forward<TValueRef>(value))
                    , Deadline(deadline)
                {
                    Y_UNUSED(hardDeadline);
                }

                TCacheItem() = default;
                TCacheItem(const TCacheItem& other) = default;
                TCacheItem(TCacheItem&& other) = default;

                [[nodiscard]]
                bool Expired(std::nullptr_t /* degradation is nullptr_t when Degrade = false */, TInstant now) const {
                    return ShouldErase(now);
                }

                bool ShouldErase(TInstant now) const {
                    return now > Deadline;
                }
            };

            struct TCacheItemWithDegradation {
                TValue Value;
                TInstant Deadline; // use a monotonic clock?
                TInstant HardDeadline;

                template<typename TValueRef>
                TCacheItemWithDegradation(TValueRef&& value, TInstant deadline, TInstant hardDeadline)
                    : Value(std::forward<TValueRef>(value))
                    , Deadline(deadline)
                    , HardDeadline(hardDeadline)
                {
                }

                TCacheItemWithDegradation() = default;
                TCacheItemWithDegradation(const TCacheItemWithDegradation& other) = default;
                TCacheItemWithDegradation(TCacheItemWithDegradation&& other) = default;

                [[nodiscard]]
                bool Expired(float degradation, TInstant now) const {
                    return now > (Deadline + degradation * (HardDeadline - Deadline));
                }

                bool ShouldErase(TInstant now) const {
                    return now > HardDeadline;
                }
            };
            using TCacheItemType = std::conditional_t<Degrade, TCacheItemWithDegradation, TCacheItem>;
            using TDegradeValue = std::conditional_t<Degrade, float, std::nullptr_t>;

            struct TDelegatingSizeProvider {
                TSizeProvider Base;
                size_t operator()(const TCacheItemType& item) {
                    return Base(item.Value);
                }
            };
            using TCacheType = TLRUCache<TKey, TCacheItemType, TNoopDelete, TDelegatingSizeProvider, TAllocator>;

        public:
            explicit TTimedLruCache(size_t maxSize)
                : Cache_(maxSize)
            {
            }

            TValue* GetPtr(const TKey& key, TDegradeValue degradation, TMaybe<TInstant> now) {
                auto iter = Cache_.Find(key);
                if (iter == Cache_.End()) {
                    return nullptr;
                }
                if (!now.Defined()) {
                    now = TInstant::Now();
                }
                if (iter->Expired(degradation, *now)) {
                    if (iter->ShouldErase(*now)) {
                        Cache_.Erase(iter);
                    }
                    return nullptr;
                }
                return &(iter->Value);
            }

            template<typename TKeyRef, typename TValueRef>
            void Put(TKeyRef&& key, TValueRef&& value, TDuration ttl, TDuration httl) {
                TInstant now = TInstant::Now();
                Put(std::forward<TKeyRef>(key), std::forward<TValueRef>(value), now + ttl, now + httl);
            }

            template<typename TKeyRef, typename TValueRef>
            void Put(TKeyRef&& key, TValueRef&& value, TInstant deadline, TInstant hardDeadline) {
                TCacheItemType item(std::forward<TValueRef>(value), deadline, hardDeadline);
                Cache_.Update(std::forward<TKeyRef>(key), std::move(item));
            }

            void Reserve(size_t hint) {
                Cache_.Reserve(hint);
            }

            using TNodeAllocatorType = typename TCacheType::TNodeAllocatorType;
            TNodeAllocatorType& GetNodeAllocator() {
                return Cache_.GetNodeAllocator();
            }

        private:
            TCacheType Cache_;
        };
    } // namespace NDetail


    // Cache key is split into two independent parts, one of which is associated with the user query and the other
    // is associated with the particular document. It makes the API a bit cumbersome to use, but actually simplifies
    // the implementation and allows to perform batch request more easily.

    template <typename TQueryKey, typename TDocKey, typename TValue, size_t ShardsCount = 16, bool Degrade = false>
    class TQueryDocLruCache {
        static_assert((ShardsCount & (ShardsCount - 1)) == 0, "ShardsCount must be a power of 2");
        using TDegradeValue = std::conditional_t<Degrade, float, std::nullptr_t>;

    public:
        TQueryDocLruCache(size_t maxSize, size_t maxDocumentCacheSize)
            : MaxShardSize_((maxSize + ShardsCount - 1) / ShardsCount)
            , MaxDocumentCacheSize_(maxDocumentCacheSize)
            , Shards_(MakeCacheShards(MaxShardSize_, std::make_index_sequence<ShardsCount>()))
        {

        }

        TQueryDocLruCache(const TQueryDocLruCacheConfig& config)
            : TQueryDocLruCache(config.GetMaxQueryCacheSize(), config.GetMaxDocumentCacheSize())
        {

        }

        /* Looks up one item in the cache, returns an empty optional if nothing has been found
         * Expired values are never returned and are treated as non-existent
         */
        template<typename TResult = TMaybe<TValue>>
        TResult Get(const TQueryKey& queryKey, const TDocKey& docKey, TDegradeValue degradation = TDegradeValue(), TMaybe<TInstant> now = Nothing()) {
            const size_t shardNumber = ShardNumber(queryKey);
            auto guard = Guard(Locks_[shardNumber]);
            TDocumentCachePtr docCache = GetDocumentCache<false>(queryKey, shardNumber);

            if (docCache) {
                TValue* valuePtr = docCache->GetPtr(docKey, degradation, now);
                if (valuePtr) {
                    return *valuePtr;
                }
            }

            return TResult();
        }

        /* Looks up a batch of documents for the given query and returns found values, if any.
         * Sizes of the input and output arrays a guaranteed to be equal, if there was nothing
         * found in the cache for the given document, the resulting TMaybe will be empty.
         */
        template<typename TResult>
        void GetSeveral(TVector<TResult>& result, const TQueryKey& queryKey, TConstArrayRef<TDocKey> docKeys, TDegradeValue degradation = TDegradeValue(), TMaybe<TInstant> now = Nothing()) {
            const size_t shardNumber = ShardNumber(queryKey);
            result.resize(docKeys.size());
            auto guard = Guard(Locks_[shardNumber]);
            TDocumentCachePtr docCache = GetDocumentCache<false>(queryKey, shardNumber);

            if (docCache) {
                for (size_t i = 0; i < docKeys.size(); ++i) {
                    TValue* valuePtr = docCache->GetPtr(docKeys[i], degradation, now);
                    if (valuePtr) {
                        result[i] = *valuePtr;
                    }
                }
            }
        }

        TVector<TMaybe<TValue>> GetSeveral(const TQueryKey& queryKey, TConstArrayRef<TDocKey> docKeys, TDegradeValue degradation = TDegradeValue(), TMaybe<TInstant> now = Nothing()) {
            TVector<TMaybe<TValue>> result;
            GetSeveral(result, queryKey, docKeys, degradation, now);
            return result;
        }

        /* Adds a new or updates an existing value. In case of an already existing value, it will
         * be overwritten completely, including the deadline (even if the new deadline is less than the current one).
         */
        template<typename TValueRef>
        void Put(const TQueryKey& queryKey, const TDocKey& docKey, TValueRef&& value, TInstant deadline, TInstant hardDeadline = TInstant::Zero()) {
            const size_t shardNumber = ShardNumber(queryKey);
            auto guard = Guard(Locks_[shardNumber]);

            TDocumentCachePtr docCache = GetDocumentCache<true>(queryKey, shardNumber);

            if (docCache) {
                if (!hardDeadline) {
                    hardDeadline = deadline;
                }
                docCache->Put(docKey, std::forward<TValueRef>(value), deadline, hardDeadline);
            }
        }

        /* Batched version of Put, adds several documents in one go. Sizes of all input arrays must be equal.
         */
        template <typename TValuesIter, bool MayMoveValues = false>
        void PutSeveral(
            const TQueryKey& queryKey,
            TConstArrayRef<TDocKey> docKeys,
            TValuesIter&& valuesBegin,
            TValuesIter&& valuesEnd,
            TConstArrayRef<TInstant> deadlines,
            TConstArrayRef<TInstant> hardDeadlines = TConstArrayRef<TInstant>())
        {
            Y_ENSURE(docKeys.size() == deadlines.size());
            Y_ENSURE((i64)docKeys.size() == std::distance(valuesBegin, valuesEnd));

            const size_t shardNumber = ShardNumber(queryKey);
            auto guard = Guard(Locks_[shardNumber]);

            TDocumentCachePtr docCache = GetDocumentCache<true>(queryKey, shardNumber);

            if (hardDeadlines.empty()) {
                hardDeadlines = deadlines;
            }

            if (docCache) {
                for (size_t i = 0; i < docKeys.size(); ++i) {
                    if constexpr (MayMoveValues) {
                        docCache->Put(docKeys[i], std::move(valuesBegin[i]), deadlines[i], hardDeadlines[i]);
                    } else {
                        docCache->Put(docKeys[i], valuesBegin[i], deadlines[i], hardDeadlines[i]);
                    }
                }
            }
        }

        inline void PutSeveral(
            const TQueryKey& queryKey,
            TConstArrayRef<TDocKey> docKeys,
            TConstArrayRef<TValue> values,
            TConstArrayRef<TInstant> deadlines,
            TConstArrayRef<TInstant> hardDeadlines = TConstArrayRef<TInstant>())
        {
            PutSeveral<const TValue*, false>(queryKey, docKeys, values.begin(), values.end(), deadlines, hardDeadlines);
        }

        inline void PutSeveralWithMove(
            const TQueryKey& queryKey,
            TConstArrayRef<TDocKey> docKeys,
            TArrayRef<TValue> values,
            TConstArrayRef<TInstant> deadlines,
            TConstArrayRef<TInstant> hardDeadlines = TConstArrayRef<TInstant>())
        {
            PutSeveral<TValue*, true>(queryKey, docKeys, values.begin(), values.end(), deadlines, hardDeadlines);
        }

    private:
        using TDocumentCache = NDetail::TTimedLruCache<TDocKey, TValue, Degrade>;
        using TDocumentCachePtr = TSimpleSharedPtr<TDocumentCache>;
        using TQueryCache = TLRUCache<TQueryKey, TDocumentCachePtr>;

        template <bool Create>
        TDocumentCachePtr GetDocumentCache(const TQueryKey& queryKey, size_t shardNumber) {
            TQueryCache& shard = Shards_[shardNumber];
            auto iter = shard.Find(queryKey);
            if (iter != shard.End()) {
                return *iter;
            }
            if constexpr (Create) {
                TDocumentCachePtr documentCache(new TDocumentCache(MaxDocumentCacheSize_));
                if (shard.Insert(queryKey, documentCache)) {
                    return documentCache;
                }
            }
            return nullptr;
        }

        static size_t ShardNumber(const TQueryKey& queryKey) {
            return THash<TQueryKey>()(queryKey) & (ShardsCount - 1);
        }

        // What have I done?
        template <size_t ... Index>
        static std::array<TQueryCache, sizeof...(Index)> MakeCacheShards(size_t shardSize, std::index_sequence<Index...>) {
            return { ((void)Index, TQueryCache(shardSize))... };
        }

    private:

        const size_t MaxShardSize_;
        const size_t MaxDocumentCacheSize_;

        std::array<TQueryCache, ShardsCount> Shards_;
        std::array<TMutex, ShardsCount> Locks_;
    };


    template <typename TDocKey, typename TValue, size_t ShardsCount = 16, bool Degrade = false, typename TSizeProvider = TUniformSizeProvider<TValue>, typename TAllocator = std::allocator<void>>
    class TDocLruCache {
        static_assert((ShardsCount & (ShardsCount - 1)) == 0, "ShardsCount must be a power of 2");
        using TDegradeValue = std::conditional_t<Degrade, float, std::nullptr_t>;

        struct TShardDocsChain {
            ui32 ShardNumber;
            ui32 FirstShardDocIndex;
        };

        struct TShardDoc {
            ui32 NextShardDocIndex;
            ui32 DocIndex;
        };

    public:
        TDocLruCache(size_t maxSize)
            : MaxShardSize_((maxSize + ShardsCount - 1) / ShardsCount)
            , Shards_(MakeCacheShards(MaxShardSize_, std::make_index_sequence<ShardsCount>()))
        {

        }

        TDocLruCache(const TDocLruCacheConfig& config)
            : TDocLruCache(config.GetMaxCacheSize())
        {

        }

        template<typename TResult = TMaybe<TValue>>
        TResult Get(const TDocKey& docKey, TDegradeValue degradation = TDegradeValue(), TMaybe<TInstant> now = Nothing()) {
            const size_t shardNumber = ShardNumber(docKey);
            auto guard = Guard(Locks_[shardNumber]);

            TValue* valuePtr = Shards_[shardNumber].GetPtr(docKey, degradation, now);
            if (valuePtr) {
                return *valuePtr;
            }
            return TResult();
        }

        template<typename TResult>
        void GetSeveral(TVector<TResult>& result, TConstArrayRef<TDocKey> docKeys, TDegradeValue degradation = TDegradeValue(), TMaybe<TInstant> now = Nothing()) {
            TVector<TShardDoc> shardDocs(::Reserve(docKeys.size()));
            std::array<TShardDocsChain, ShardsCount> shardDocsChains = MakeShardDocsChains(std::make_index_sequence<ShardsCount>());

            for (ui32 i = 0; i < docKeys.size(); ++i) {
                const size_t shardNumber = ShardNumber(docKeys[i]);
                shardDocs.emplace_back(TShardDoc { shardDocsChains[shardNumber].FirstShardDocIndex, i });
                shardDocsChains[shardNumber].FirstShardDocIndex = shardDocs.size() - 1;
            }

            Shuffle(shardDocsChains.begin(), shardDocsChains.end());

            result.resize(docKeys.size());

            for (const TShardDocsChain& shardDocsChain : shardDocsChains) {
                if (shardDocsChain.FirstShardDocIndex == Max<ui32>()) {
                    continue;
                }

                auto guard = Guard(Locks_[shardDocsChain.ShardNumber]);

                ui32 shardDocIndex = shardDocsChain.FirstShardDocIndex;
                while (shardDocIndex != Max<ui32>()) {
                    const TShardDoc& shardDoc = shardDocs[shardDocIndex];
                    TValue* value = Shards_[shardDocsChain.ShardNumber].GetPtr(docKeys[shardDoc.DocIndex], degradation, now);
                    if (value) {
                        result[shardDoc.DocIndex] = *value;
                    }
                    shardDocIndex = shardDoc.NextShardDocIndex;
                }
            }
        }

        TVector<TMaybe<TValue>> GetSeveral(TConstArrayRef<TDocKey> docKeys, TDegradeValue degradation = TDegradeValue(), TMaybe<TInstant> now = Nothing()) {
            TVector<TMaybe<TValue>> result;
            GetSeveral(result, docKeys, degradation, now);
            return result;
        }

        template<typename TValueRef>
        void Put(const TDocKey& docKey, TValueRef&& value, TInstant deadline, TInstant hardDeadline = TInstant::Zero()) {
            const size_t shardNumber = ShardNumber(docKey);
            auto guard = Guard(Locks_[shardNumber]);
            if (hardDeadline == TInstant::Zero()) {
                hardDeadline = deadline;
            }
            Shards_[shardNumber].Put(docKey, std::forward<TValueRef>(value), deadline, hardDeadline);
        }

        template <typename TValuesIter, bool MayMoveValues = false>
        void PutSeveral(
            TConstArrayRef<TDocKey> docKeys,
            TValuesIter&& valuesBegin,
            TValuesIter&& valuesEnd,
            TConstArrayRef<TInstant> deadlines,
            TConstArrayRef<TInstant> hardDeadlines = TConstArrayRef<TInstant>())
        {
            Y_ENSURE(docKeys.size() == deadlines.size());
            Y_ENSURE((i64)docKeys.size() == std::distance(valuesBegin, valuesEnd));

            TVector<TShardDoc> shardDocs(::Reserve(docKeys.size()));
            std::array<TShardDocsChain, ShardsCount> shardDocsChains = MakeShardDocsChains(std::make_index_sequence<ShardsCount>());

            for (ui32 i = 0; i < docKeys.size(); ++i) {
                const size_t shardNumber = ShardNumber(docKeys[i]);
                shardDocs.emplace_back(TShardDoc { shardDocsChains[shardNumber].FirstShardDocIndex, i });
                shardDocsChains[shardNumber].FirstShardDocIndex = shardDocs.size() - 1;
            }

            Shuffle(shardDocsChains.begin(), shardDocsChains.end());

            if (hardDeadlines.empty()) {
                hardDeadlines = deadlines;
            }
            for (const TShardDocsChain& shardDocsChain : shardDocsChains) {
                if (shardDocsChain.FirstShardDocIndex == Max<ui32>()) {
                    continue;
                }

                auto guard = Guard(Locks_[shardDocsChain.ShardNumber]);

                ui32 shardDocIndex = shardDocsChain.FirstShardDocIndex;
                while (shardDocIndex != Max<ui32>()) {
                    const TShardDoc& shardDoc = shardDocs[shardDocIndex];
                    if constexpr (MayMoveValues) {
                        Shards_[shardDocsChain.ShardNumber].Put(docKeys[shardDoc.DocIndex], std::move(valuesBegin[shardDoc.DocIndex]), deadlines[shardDoc.DocIndex], hardDeadlines[shardDoc.DocIndex]);
                    } else {
                        Shards_[shardDocsChain.ShardNumber].Put(docKeys[shardDoc.DocIndex], valuesBegin[shardDoc.DocIndex], deadlines[shardDoc.DocIndex], hardDeadlines[shardDoc.DocIndex]);
                    }
                    shardDocIndex = shardDoc.NextShardDocIndex;
                }
            }
        }

        inline void PutSeveral(
            TConstArrayRef<TDocKey> docKeys,
            TConstArrayRef<TValue> values,
            TConstArrayRef<TInstant> deadlines,
            TConstArrayRef<TInstant> hardDeadlines = TConstArrayRef<TInstant>())
        {
            PutSeveral<const TValue*, false>(docKeys, values.begin(), values.end(), deadlines, hardDeadlines);
        }

        inline void PutSeveralWithMove(
            TConstArrayRef<TDocKey> docKeys,
            TArrayRef<TValue> values,
            TConstArrayRef<TInstant> deadlines,
            TConstArrayRef<TInstant> hardDeadlines = TConstArrayRef<TInstant>())
        {
            PutSeveral<TValue*, true>(docKeys, values.begin(), values.end(), deadlines, hardDeadlines);
        }

        void Reserve(size_t hint) {
            for (TQueryCache& c : Shards_)
                c.Reserve((hint + ShardsCount - 1) / ShardsCount + 1); // +1 because TLRUCache::Insert adds a new item before removing an old one
        }

        template<typename T>
        void ForShardNodeAllocators(const T& functor) {
            for (TQueryCache& c : Shards_)
                functor(c.GetNodeAllocator());
        }

    private:
        static size_t ShardNumber(const TDocKey& docKey) {
            return THash<TDocKey>()(docKey) & (ShardsCount - 1);
        }

    private:
        using TQueryCache = NDetail::TTimedLruCache<TDocKey, TValue, Degrade, TSizeProvider, TAllocator>;

    public:
        using TNodeAllocatorType = typename TQueryCache::TNodeAllocatorType;

    private:
        template <size_t ... Index>
        static std::array<TQueryCache, sizeof...(Index)> MakeCacheShards(size_t shardSize, std::index_sequence<Index...>) {
            return { ((void)Index, TQueryCache(shardSize))... };
        }

        template <size_t ... Index>
        static constexpr std::array<TShardDocsChain, sizeof...(Index)> MakeShardDocsChains(std::index_sequence<Index...>) {
            return { TShardDocsChain({ Index, Max<ui32>() })... };
        }

    private:

        const size_t MaxShardSize_;

        std::array<TQueryCache, ShardsCount> Shards_;
        std::array<TMutex, ShardsCount> Locks_;
    };
}
