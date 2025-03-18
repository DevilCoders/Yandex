#pragma once

#include "cbb.h"
#include "config_global.h"
#include "eventlog_err.h"

#include <antirobot/lib/alarmer.h>

#include <library/cpp/iterator/iterate_keys.h>
#include <library/cpp/iterator/zip.h>

#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/synchronized/synchronized.h>

#include <util/datetime/base.h>
#include <util/digest/sequence.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/mutex.h>

#include <atomic>
#include <functional>
#include <type_traits>
#include <utility>


namespace NAntiRobot {


class ICbbListManager {
public:
    virtual void Update(
        ICbbIO* cbb,
        const TVector<std::pair<TCbbGroupId, TInstant>>& idToTimestamp
    ) = 0;

    virtual TVector<TCbbGroupId> GetIds() const = 0;
};


struct TCbbListManagerCallback {
    TStringBuf GetTitle() const {
        return {};
    }
};


// The TCbbIO that is passed to Update must be destroyed before TCbbListManager.
template <typename TRequester, typename TCallback>
class TCbbListManager : public ICbbListManager {
public:
    struct TCallbackData {
        TInstant Timestamp;
        TCallback Callback;
    };

public:
    void Update(
        ICbbIO* cbb,
        const TVector<std::pair<TCbbGroupId, TInstant>>& idToTimestamp
    ) override {
        DoUpdate(cbb, idToTimestamp);
    }

    TVector<TCbbGroupId> GetIds() const override {
        const auto guard = Lock();
        const auto idRange = IterateKeys(IdToList);
        return {idRange.begin(), idRange.end()};
    }

    template <typename F>
    void IterateCallbacks(F&& f) {
        // Copy the pointers and perform merging outside the critical section so updates don't have
        // to wait until merging is complete.
        TVector<TAtomicSharedPtr<NThreading::TSynchronized<TCallbackData>>> callbackDataPtrs;

        with_lock (Mutex) {
            for (const auto& [_, callbackEntry] : IdsToCallback) {
                callbackDataPtrs.push_back(callbackEntry.Data);
            }
        }

        for (const auto& callbackDataPtr : callbackDataPtrs) {
            auto callbackData = callbackDataPtr->Access();
            f(callbackData->Callback);
        }
    }

protected:
    std::pair<
        TGuard<TMutex>,
        typename NThreading::TSynchronized<TCallbackData>::TAccess
    > Add(const TVector<TCbbGroupId>& ids) {
        auto guard = Lock();
        const auto [idsCallbackData, _] = IdsToCallback.try_emplace(ids);
        auto& callbackEntry = idsCallbackData->second;

        ++callbackEntry.NumRefs;

        if (callbackEntry.NumRefs == 1) {
            for (const auto id : ids) {
                ++IdToList[id].NumRefs;
            }
        }

        return {std::move(guard), callbackEntry.Data->Access()};
    }

    void Remove(const TVector<TCbbGroupId>& ids, size_t numRefs = 1) {
        const auto guard = Lock();
        const auto idsCallbackData = IdsToCallback.find(ids);

        if (idsCallbackData == IdsToCallback.end()) {
            return;
        }

        idsCallbackData->second.NumRefs -= Min(numRefs, idsCallbackData->second.NumRefs);

        if (idsCallbackData->second.NumRefs > 0) {
            return;
        }

        IdsToCallback.erase(idsCallbackData);

        for (const auto id : ids) {
            if (const auto idData = IdToList.find(id); idData != IdToList.end()) {
                --idData->second.NumRefs;

                if (idData->second.NumRefs == 0) {
                    IdToList.erase(idData);
                }
            }
        }
    }

    TGuard<TMutex> Lock() const {
        return Guard(Mutex);
    }

private:
    void DoUpdate(
        ICbbIO* cbb,
        const TVector<std::pair<TCbbGroupId, TInstant>>& idToTimestamp,
        bool retry = false
    ) {
        if (idToTimestamp.empty()) {
            return;
        }

        const auto guard = Lock();

        THashSet<TCbbGroupId> updatedIds;
        updatedIds.reserve(idToTimestamp.size());

        TVector<TFuture<TString>> updatedFutures;
        updatedFutures.reserve(idToTimestamp.size());

        TVector<std::pair<TCbbGroupId, TInstant>> updatedIdToTimestamp;
        updatedIdToTimestamp.reserve(idToTimestamp.size());

        for (const auto& [id, timestamp] : idToTimestamp) {
            const auto listEntry = IdToList.FindPtr(id);

            if (!listEntry) {
                continue;
            }

            auto listData = listEntry->Data->Access();

            if (
                retry ?
                    timestamp < listData->Timestamp :
                    timestamp <= listData->Timestamp
            ) {
                continue;
            }

            const auto future = TRequester()(cbb, id);

            listData->Timestamp = timestamp;
            listData->Future = future;

            updatedIds.insert(id);
            updatedFutures.push_back(std::move(future));
            updatedIdToTimestamp.push_back({id, timestamp});
        }

        if (updatedIds.empty()) {
            return;
        }

        for (const auto& [ids, callbackEntry] : IdsToCallback) {
            if (!AnyOf(ids, [&updatedIds] (const auto id) {
                return updatedIds.contains(id);
            })) {
                continue;
            }

            TVector<TFuture<TString>> futures;
            futures.reserve(ids.size());

            TInstant maxTimestamp;

            for (const auto id : ids) {
                auto listData = IdToList.FindPtr(id)->Data->Access();
                futures.push_back(listData->Future);
                maxTimestamp = Max(maxTimestamp, listData->Timestamp);
            }

            NThreading::WaitAll(futures).Subscribe([
                futures = std::move(futures),
                maxTimestamp,
                callbackDataAccessor = callbackEntry.Data
            ] (const auto&) {
                auto callbackData = callbackDataAccessor->Access();

                if (maxTimestamp <= callbackData->Timestamp) {
                    return;
                }

                bool ok = true;

                TVector<TString> result;
                result.reserve(futures.size());

                for (const auto& future : futures) {
                    try {
                        result.push_back(future.GetValue());
                    } catch (...) {
                        ok = false;
                        break;
                    }
                }

                if (ok) {
                    callbackData->Timestamp = maxTimestamp;
                    RunCallback(callbackData->Callback, result);
                }
            });
        }

        NThreading::WaitAll(updatedFutures).Subscribe([
            this,
            cbb,
            updatedFutures = std::move(updatedFutures),
            updatedIdToTimestamp = std::move(updatedIdToTimestamp)
        ] (const auto&) {
            try {
                TVector<std::pair<TCbbGroupId, TInstant>> failedIdToTimestamp;

                for (
                    const auto& [future, idTimestamp] :
                        Zip(updatedFutures, updatedIdToTimestamp)
                ) {
                    if (future.HasException()) {
                        failedIdToTimestamp.push_back(idTimestamp);
                    }
                }

                DoUpdate(cbb, failedIdToTimestamp, true);
            } catch (...) {}
        });
    }

    static void RunCallback(
        TCallback& callback,
        const TVector<TString>& result
    ) {
        try {
            callback(result);
        } catch (const yexception& exc) {
            auto msg = EVLOG_MSG;
            msg << EVLOG_ERROR << "Exception while updating list";

            if (const auto& title = callback.GetTitle(); !title.empty()) {
                msg << ": " << title;
            }

            msg << ": " << exc.what();
        }
    }

private:
    struct TListData {
        TInstant Timestamp;
        TFuture<TString> Future;
    };

    struct TListEntry {
        size_t NumRefs = 0;
        TAtomicSharedPtr<NThreading::TSynchronized<TListData>> Data =
            MakeAtomicShared<NThreading::TSynchronized<TListData>>();
    };

    struct TCallbackEntry {
        size_t NumRefs = 0;
        TAtomicSharedPtr<NThreading::TSynchronized<TCallbackData>> Data =
            MakeAtomicShared<NThreading::TSynchronized<TCallbackData>>();
    };

    TMutex Mutex;
    THashMap<TCbbGroupId, TListEntry> IdToList;
    THashMap<TVector<TCbbGroupId>, TCallbackEntry, TRangeHash<>> IdsToCallback;
};


} // namespace NAntiRobot
