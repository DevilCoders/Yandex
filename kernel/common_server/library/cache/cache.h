#pragma once

#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/settings/abstract/abstract.h>

namespace NLogistic {
    template<class TElement>
    class TCommonCache : public IAutoActualization {
        using TBase = IAutoActualization;
        using TBase::TBase;

        class TCachedElements {
            CSA_DEFAULT(TCachedElements, TElement, Value);
            CSA_DEFAULT(TCachedElements, TInstant, AcquisitionInstant);
        };

        class TCleanupQueueItem {
            CSA_DEFAULT(TCleanupQueueItem, TCachedElements, CachedElement);
            CSA_DEFAULT(TCleanupQueueItem, TString, Key);

        public:
            TCleanupQueueItem(const TCachedElements& cachedElement, const TString& key)
                : CachedElement(cachedElement)
                , Key(key)
            {
            }
        };

    public:
        TCommonCache(const TString& cacheName)
            : TBase(cacheName)
            , CacheName(cacheName)
        {
        }

        TMaybe<TElement> GetCachedElement(const TString& key) const {
            TReadGuard rg(Mutex);
            auto result = GetCachedElementUnsafe(key, rg);
            TCSSignals::Signal("common.cache." + CacheName + "." + (result.Defined() ? "reused" : "discarded"));
            return result;
        }

        void AddCachedElement(const TElement& value, const TString& key, const TInstant actuality) {
            TWriteGuard wg(Mutex);
            AddCachedElementUnsafe(value, key, actuality, wg);
        }

        virtual bool Refresh() override {
            TWriteGuard wg(Mutex);
            if (CleanupQueue.empty()) {
                return true;
            }
            const auto windowSize = ICSOperator::HasServer() ? Min(GetMaxTimeDiff(), ICSOperator::GetServer().GetSettings().GetValueDef<TDuration>("common.cache.max_window_size", TDuration::Minutes(30))) : GetMaxTimeDiff();
            const auto outdatedUntil = Now() - windowSize;

            size_t erasedElements = 0;
            size_t outdatedElements = 0;
            while (!CleanupQueue.empty() && CleanupQueue.front().GetCachedElement().GetAcquisitionInstant() < outdatedUntil) {
                const auto element = CleanupQueue.front();
                CleanupQueue.pop();
                const auto key = element.GetKey();
                auto it = CachedElements.find(key);
                if (it != CachedElements.end() && it->second.GetAcquisitionInstant() == element.GetCachedElement().GetAcquisitionInstant()) {
                    CachedElements.erase(it);
                    ++erasedElements;
                } else {
                    ++outdatedElements;
                }
            }

            TCSSignals::Signal("common.cache." + CacheName + ".erased_elements", erasedElements);
            TCSSignals::Signal("common.cache." + CacheName + ".outdated_elements", outdatedElements);
            TCSSignals::Signal("common.cache." + CacheName + ".cleanup_queue_size", CleanupQueue.size());
            TCSSignals::Signal("common.cache." + CacheName + ".cached_elements_size", CachedElements.size());

            return true;
        }

    private:
        TDuration GetMaxTimeDiff() const {
            return ICSOperator::HasServer() ? ICSOperator::GetServer().GetSettings().GetValueDef<TDuration>("common.cache." + CacheName + ".max_time_diff", TDuration::Minutes(10)) : TDuration::Minutes(10);
        }

        TMaybe<TElement> GetCachedElementUnsafe(const TString& key, const TReadGuard& /*rg*/) const {
            auto it = CachedElements.find(key);
            if (it == CachedElements.end()) {
                TCSSignals::Signal("common.cache." + CacheName + ".discard_reason")("reason", "absent");
                return Nothing();
            }
            auto timeDiff = Now() - it->second.GetAcquisitionInstant();

            bool isOutdated = timeDiff > GetMaxTimeDiff();
            if (isOutdated) {
                TCSSignals::Signal("common.cache." + CacheName + ".discard_reason")("reason", "outdated");
                return Nothing();
            }

            return it->second.GetValue();
        }

        void AddCachedElementUnsafe(const TElement& value, const TString& key, const TInstant actuality, TWriteGuard& /*wg*/) {
            TCachedElements dst;
            dst.SetAcquisitionInstant(actuality);
            dst.SetValue(value);

            CleanupQueue.emplace(dst, key);
            CachedElements[key] = std::move(dst);

            TCSSignals::Signal("common.cache." + CacheName + ".updated");
        }

        TString CacheName;
        TRWMutex Mutex;
        TMap<TString, TCachedElements> CachedElements;
        TQueue<TCleanupQueueItem> CleanupQueue;
    };
}
