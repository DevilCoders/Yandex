#pragma once

#include <tools/clustermaster/common/log.h>

#include <tools/clustermaster/proto/state_row.pb.h>

#include <kernel/yt/dynamic/table.h>
#include <mapreduce/yt/interface/fwd.h>

#include <util/folder/path.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/fs.h>

#include <mutex>
#include <shared_mutex>

void SplitByOneOf(const TString& str, TStringBuf delim, TStringBuf& first, TStringBuf& rest);

inline bool IsTrueValue(const TString& val) {
    return (val == "true" || val == "yes" || FromString<int>(val) != 0);
}

inline TString ReplaceAll(TStringBuf src, TStringBuf from, TStringBuf to) {
    TString result;
    size_t start = 0;
    size_t end = 0;

    while ((end = src.find(from, end)) != TStringBuf::npos) {
        result += src.substr(start, end);
        result += to;
        end += from.size();
        start = end;
    }
    result += src.substr(start, end);
    return result;
}

const TString ReadYtToken(const TString& path);

class TYtConnectionStatus {
    ui64 BackoffPeriodMin;
    ui64 BackoffPeriodMax;
    mutable std::shared_mutex Mutex;
    TInstant LastFailure;
    TInstant NextCheckTime;
    ui64 WaitPeriod = 0;
    ui64 RemainingSeconds = 0;

    bool CheckWaitPeriod();
    bool CheckRemainingSeconds();
    void Enable();
    bool UpdateRemainingSeconds();

public:
    TYtConnectionStatus(ui64 backoffPeriodMin, ui64 backoffPeriodMax) :
        BackoffPeriodMin(backoffPeriodMin),
        BackoffPeriodMax(backoffPeriodMax)
    {
        Y_ENSURE(BackoffPeriodMin >= 10,
                 "BackoffPeriodMin must be at least 10 seconds");
        Y_ENSURE(BackoffPeriodMax >= BackoffPeriodMin,
                 "BackoffPeriodMax must be greater or equal to BackoffPeriodMin");
    }

    void Disable(const char* reason);
    void EnableIfNeeded();
    bool Disabled();
};

class TYtStorage {
    TString YtDirPath;
    TString TableName;
    NYT::NApi::IClientPtr Client;
    NYT::NProtoApi::TTable<NProto::TStateRow> YtStateTable;
    TYtConnectionStatus Status;

    mutable std::shared_mutex Mutex;
    THashMap<TString, TString> Updates;

    void Setup() const;
    int CountUpdates();
    void AddUpdate(const TString& key, const TString& value);
    bool ExecuteRequest(std::function<void()> request);
    bool WriteRow(const TString& key, const TString& value);
    bool Flush();

public:
    TYtStorage(
        const TString& ytDirPath,
        const TString& tableName,
        const NYT::NApi::IClientPtr &client,
        ui64 backoffPeriodMin,
        ui64 backoffPeriodMax
    ) :
        YtDirPath(ytDirPath),
        TableName(tableName),
        Client(client),
        YtStateTable(NJupiter::JoinYtPath(ytDirPath, tableName), client),
        Status(backoffPeriodMin, backoffPeriodMax)
    {
        Setup();
    }

    bool Save(const TString& key, const TString& value);
    TMaybe<TString> Load(const TString& key);

    template<class T>
    bool SaveProtoStateToYt(const TString& key, const T& protoState) {
        try {
            TString str;
            if (!protoState.SerializeToString(&str)) {
                ythrow yexception() << "Serializing of state (or variables) to string failed";
            }
            return Save(key, str);
        } catch (const NYT::TErrorException& e) {
            ERRORLOG("Failed to save state to YT: " << e.what());
            return false;
        }
    }

    template<class T>
    TMaybe<T> LoadAndParseStateFromYt(const TString& key) {
        TMaybe<TString> str = Load(key);
        if (!str) {
            ERRORLOG("Failed to retrieve state from YT at key " << key);
            return {};
        }
        T protoState;
        if (!protoState.ParseFromString(str.GetRef())) {
            ERRORLOG("Parsing of string retrieved from YT at key " << key << " has failed");
            return {};
        }
        return MakeMaybe(std::move(protoState));
    }
};

void PrintSimpleHttpHeader(IOutputStream& out, TStringBuf contentType);
TVector<TString> SplitRecipients(const TString& str);

template<class T>
void SaveProtoStateToDisk(const TFsPath& fsPath, const T& protoState) {
    const TString& path = fsPath.GetPath();
    try {
        fsPath.Parent().MkDirs();
        TFile file(path, OpenAlways | WrOnly);
        file.Flock(LOCK_EX);

        TUnbufferedFileOutput out(file);
        if (!protoState.SerializeToArcadiaStream(&out)) {
            ythrow yexception() << "Serializing of state (or variables) to stream failed";
        }
        // Truncate file if older size was larger
        file.Resize(protoState.ByteSize());
        file.Flush();
    } catch (const yexception& e) {
        NFs::Remove(path);
        ERRORLOG("Failed to save state (or variables) to file " << path << ": " << e.what());
        throw e;
    }
}

template<class T>
TMaybe<T> LoadAndParseStateFromDisk(const TFsPath& fsPath) {
    const TString& path = fsPath.GetPath();
    if (!fsPath.Exists()) {
        DEBUGLOG("Cannot load state (or variables) from nonexisting file " << path);
        return {};
    }
    try {
        TFile file(path, OpenExisting | RdOnly | Seq);
        file.Flock(LOCK_SH);

        TUnbufferedFileInput in(file);
        T protoState;
        if (!protoState.ParseFromArcadiaStream(&in)) {
            ERRORLOG("Parsing of state (or variables) retrieved from file " << path << " has failed");
            return {};
        }
        return MakeMaybe(std::move(protoState));
    } catch (const yexception& e) {
        NFs::Remove(path);
        ERRORLOG("Failed to load state (or variables) from " << path << ": " << e.what());
        return {};
    }
}

/* T is protobuf message with UpdateTimestamp field */
template<class T>
TMaybe<T> SelectByUpdateTimestamp(const TVector<TMaybe<T>>& candidates) {
    T newest;
    bool found = false;
    for (const TMaybe<T>& candidate : candidates) {
        if (candidate.Empty())
            continue;
        if (!found || newest.GetUpdateTimestamp() < candidate->GetUpdateTimestamp()) {
            newest.CopyFrom(*candidate);
            found = true;
        }
    }
    if (!found) {
        return {};
    }
    return MakeMaybe(std::move(newest));
}
