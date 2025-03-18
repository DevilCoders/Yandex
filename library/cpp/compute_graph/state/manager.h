#pragma once

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/system/mutex.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>

namespace NComputeGraph {
    class TTaskId {
        TVector<TString> Id;

    public:
        TTaskId() {
        }

        TTaskId(const TString& id);

        TTaskId(const char* id);

        TTaskId(const TVector<TString>& ids);

        TTaskId operator/(const TString& s) const;

        bool operator==(const TTaskId& other) const;

        bool Empty() const;

        static TTaskId FromString(const TStringBuf& s);

        TString Str() const;

        size_t GetHash() const;

    private:
        void CheckEmptyAndTabs(const TString& s) const;
    };

    struct TTaskIdHash {
        inline size_t operator()(const TTaskId& x) {
            return x.GetHash();
        }
    };

    class TStateManager: public TThrRefBase {
    protected:
        TMutex M;
        THashMap<TTaskId, size_t, TTaskIdHash> FinishedTasks;
        THashSet<TTaskId, TTaskIdHash> StartedTasks;
        THashSet<TTaskId, TTaskIdHash> FailedTasks;

    public:
        TStateManager();

        bool StartTask(const TTaskId& task);

        void FailTask(const TTaskId& task);

        virtual void FinishTask(const TTaskId& task);

        bool HasFinished(const TTaskId& task);

        bool HasFailed(const TTaskId& task);

        bool HasStarted(const TTaskId& task);

        virtual void Clear();
    };
    using TStateManagerPtr = TIntrusivePtr<TStateManager>;

    class TFileStateManager: public TStateManager {
        TString StateFile;

    public:
        TFileStateManager(const TString& filePath);

        void FinishTask(const TTaskId& task) override;

        void Clear() override;
    };

}
