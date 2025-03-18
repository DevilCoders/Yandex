#include "manager.h"

#include <util/system/mutex.h>
#include <util/system/guard.h>
#include <util/system/pipe.h>
#include <util/string/cast.h>
#include <util/generic/string.h>
#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>
#include <util/string/vector.h>
#include <util/draft/date.h>
#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/output.h>

#include <queue>

namespace NComputeGraph {
    TTaskId::TTaskId(const TString& id) {
        if (id) {
            Id = {id};
            CheckEmptyAndTabs(id);
        }
    }

    TTaskId::TTaskId(const char* id)
        : TTaskId(TString(id))
    {
    }

    TTaskId::TTaskId(const TVector<TString>& ids)
        : Id(ids)
    {
        for (const auto& id : ids) {
            CheckEmptyAndTabs(id);
        }
    }

    TTaskId TTaskId::operator/(const TString& s) const {
        CheckEmptyAndTabs(s);
        auto res = Id;
        res.push_back(s);
        return res;
    }

    bool TTaskId::operator==(const TTaskId& other) const {
        if (other.Id.size() != Id.size()) {
            return false;
        }
        for (auto i : xrange(Id.size())) {
            if (Id[i] != other.Id[i]) {
                return false;
            }
        }
        return true;
    }

    bool TTaskId::Empty() const {
        return Id.empty();
    }

    TTaskId TTaskId::FromString(const TStringBuf& s) {
        TTaskId res;
        TStringBuf b = s;
        while (b) {
            res.Id.push_back(TString(b.NextTok('\t')));
        }
        return res;
    }

    TString TTaskId::Str() const {
        return JoinStrings(Id.begin(), Id.end(), "\t");
    }

    size_t TTaskId::GetHash() const {
        size_t hash = 0;
        for (const auto& item : Id) {
            hash = CombineHashes(hash, THash<TString>()(item));
        }
        return hash;
    }

    void TTaskId::CheckEmptyAndTabs(const TString& s) const {
        if (s.empty()) {
            throw yexception() << "Empty ids not allowed" << Endl;
        }
        if (Find(s.cbegin(), s.cend(), '\t') != s.end()) {
            throw yexception() << "Tabs are not allowed in task ids" << Endl;
        }
    }

    TStateManager::TStateManager() {
    }

    bool TStateManager::StartTask(const TTaskId& task) {
        TGuard<TMutex> g(M);
        if (!StartedTasks.contains(task) && !FinishedTasks.contains(task) && !FailedTasks.contains(task)) {
            StartedTasks.insert(task);
            return true;
        } else {
            return false;
        }
    }

    void TStateManager::FailTask(const TTaskId& task) {
        TGuard<TMutex> g(M);
        StartedTasks.erase(task);
        FailedTasks.insert(task);
    }

    void TStateManager::FinishTask(const TTaskId& task) {
        TGuard<TMutex> g(M);
        FinishedTasks[task] = TInstant::Now().TimeT();
    }

    bool TStateManager::HasFinished(const TTaskId& task) {
        TGuard<TMutex> g(M);
        return FinishedTasks.contains(task);
    }

    bool TStateManager::HasFailed(const TTaskId& task) {
        TGuard<TMutex> g(M);
        return FailedTasks.contains(task);
    }

    bool TStateManager::HasStarted(const TTaskId& task) {
        TGuard<TMutex> g(M);
        return StartedTasks.contains(task);
    }

    void TStateManager::Clear() {
        TGuard<TMutex> g(M);
        FinishedTasks.clear();
        StartedTasks.clear();
        FailedTasks.clear();
    }

    TFileStateManager::TFileStateManager(const TString& filePath)
        : StateFile(filePath)
    {
        TFsPath(filePath).Parent().MkDirs();
        if (TFsPath(filePath).Exists()) {
            TFileInput f(StateFile);
            TString line;
            while (f.ReadLine(line)) {
                TStringBuf b(line);
                TStringBuf finishTimeStr = b.NextTok('\t');
                ui64 finishTime = 0;
                if (finishTimeStr) {
                    finishTime = FromString<size_t>(finishTimeStr);
                }
                FinishedTasks[TTaskId::FromString(b)] = finishTime;
            }
        }
    }

    void TFileStateManager::FinishTask(const TTaskId& task) {
        TStateManager::FinishTask(task);

        TGuard<TMutex> g(M);
        TFile f(StateFile, OpenAlways | WrOnly | Seq | ForAppend);
        TFileOutput out(f);
        out << FinishedTasks[task.Str()] << "\t" << task.Str() << Endl;
    }

    void TFileStateManager::Clear() {
        throw yexception() << "TFileStateManager is persistent and should not be cleared";
    }

}
