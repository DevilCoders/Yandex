#pragma once

#include <kernel/common_server/util/auto_actualization.h>

#include <kernel/common_server/library/storage/abstract.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/util/mt_snapshots_collection.h>

template <class TSnapshot>
class TKVSnapshot: public IAutoActualization {
private:
    using TBase = IAutoActualization;
    using TKey = typename TSnapshot::TKey;
    using TValue = typename TSnapshot::TValue;
    NRTProc::IVersionedStorage::TPtr VStorage;
    TSwitchSnapshotsCollection<TSnapshot, 3> SnapshotCollection;
    TString CommonFolder;
public:
    TKVSnapshot(NRTProc::IVersionedStorage::TPtr vStorage, const TString& name, const TDuration frq, const TString& commonFolder = "")
        : TBase(name, frq)
        , VStorage(vStorage)
        , CommonFolder(commonFolder)
    {
        CHECK_WITH_LOG(vStorage) << "Incorrect storage name for kv-snapshot: " << name << Endl;
    }

    TAtomicSharedPtr<TSnapshot> GetCurrentSnapshot() const {
        return SnapshotCollection.Get();
    }

    virtual bool GetKeys(TSet<TString>& keys) const = 0;
    virtual TMaybe<TValue> OnRecord(const TString& key, const TString& value) const = 0;
    virtual void BeforeRefresh() {

    }
    virtual bool Refresh() override {
        TSet<TString> keysOriginal;
        TUnistatSignalsCache::Signal(GetName())("category", "status")("code", "start");
        auto guard = TUnistatSignalsCache::LSignal(GetName())("category", "status")("code", "running")(0);
        TUnistatSignalsCache::LSignal(GetName())("category", "status")("code", "running")(1);
        if (!GetKeys(keysOriginal)) {
            TUnistatSignalsCache::SignalProblem(GetName())("code", "get_keys");
            return false;
        }
        TUnistatSignalsCache::LSignal(GetName())("code", "keys_count")(keysOriginal.size());
        TSet<TString> keys;
        if (CommonFolder) {
            for (auto&& i : keysOriginal) {
                keys.emplace(CommonFolder + "/" + i);
            }
        } else {
            keys = std::move(keysOriginal);
        }
        TVector<TString> currentKeysPack;
        TVector<NRTProc::TSearchReply> sendResults;
        BeforeRefresh();
        TVector<NRTProc::IVersionedStorage::TKValue> values;
        if (!VStorage->GetValues(keys, values)) {
            TUnistatSignalsCache::SignalProblem(GetName())("code", "get_values");
            return false;
        }
        TUnistatSignalsCache::LSignal(GetName())("code", "values_count")(values.size());
        TAtomicSharedPtr<TSnapshot> nextSnapshot = new TSnapshot();
        for (auto&& i : values) {
            try {
                const TString keyOriginal = i.GetKey().substr(CommonFolder.size() + 1);
                auto recordValue = OnRecord(keyOriginal, i.GetValue());
                if (recordValue) {
                    nextSnapshot->Add(keyOriginal, *recordValue);
                } else {
                    TUnistatSignalsCache::SignalProblem(GetName())("code", "cannot_build_record");
                }
            } catch (...) {
                TUnistatSignalsCache::SignalProblem(GetName())("code", "exception");
                ERROR_LOG << CurrentExceptionMessage() << Endl;
            }
        }
        TUnistatSignalsCache::Signal(GetName())("code", "records_count")(nextSnapshot->Size());
        AfterRefresh();
        SnapshotCollection.Switch(nextSnapshot);
        return true;
    }
    virtual void AfterRefresh() {

    }
};
