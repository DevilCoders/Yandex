#include "util.h"

#include <kernel/yt/dynamic/client.h>
#include <kernel/yt/utils/yt_utils.h>
#include <yt/yt/client/object_client/public.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/string/split.h>

void SplitByOneOf(const TString& str, TStringBuf delim, TStringBuf& first, TStringBuf& rest) {
    size_t endfirst = str.find_first_of(delim);
    if (endfirst == TString::npos) {
        first = str;
    } else {
        first = TStringBuf(str.data(), str.data() + endfirst);
        size_t startrest = str.find_first_not_of(delim, endfirst);

        if (startrest != TString::npos) {
            rest = TStringBuf(str.data() + startrest, str.data() + str.size());
        }
    }
}

const TString ReadYtToken(const TString& path) {
    TFile file(path, OpenExisting | RdOnly | Seq);
    TUnbufferedFileInput in(file);
    TString token = in.ReadLine();
    LOG("Read token of length " << token.size() << " from " << path);
    return token;
}

void PrintSimpleHttpHeader(IOutputStream& out, TStringBuf contentType) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: " << contentType << "\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n\r\n";
}

bool TYtConnectionStatus::CheckWaitPeriod() {
    std::shared_lock lock(Mutex);
    return WaitPeriod == 0;
}

bool TYtConnectionStatus::CheckRemainingSeconds() {
    std::shared_lock lock(Mutex);
    return RemainingSeconds == 0;
}

void TYtConnectionStatus::Disable(const char* reason) {
    std::unique_lock lock(Mutex);
    if (WaitPeriod == 0) {
        WaitPeriod = BackoffPeriodMin;
    } else {
        WaitPeriod = std::min(WaitPeriod * 2, BackoffPeriodMax);
    }
    LastFailure = TInstant::Now();
    NextCheckTime = LastFailure + TDuration::Seconds(WaitPeriod);
    RemainingSeconds = WaitPeriod;
    ERRORLOG("YT requests are disabled for the next " << WaitPeriod << " seconds "
        << "due to: " << reason);
}

void TYtConnectionStatus::Enable() {
    std::unique_lock lock(Mutex);
    WaitPeriod = 0;
    RemainingSeconds = 0;
    LOG("YT requests are enabled again after successfull connection");
}

void TYtConnectionStatus::EnableIfNeeded() {
    if (CheckWaitPeriod()) {
        return;
    }
    Enable();
}

bool TYtConnectionStatus::UpdateRemainingSeconds() {
    std::unique_lock lock(Mutex);
    TInstant now = TInstant::Now();
    if (now >= NextCheckTime) {
        RemainingSeconds = 0;
    } else {
        RemainingSeconds = (NextCheckTime - now).Seconds();
    }
    return RemainingSeconds == 0;
}

bool TYtConnectionStatus::Disabled() {
    bool ok = CheckRemainingSeconds() || UpdateRemainingSeconds();
    return !ok;
}

void TYtStorage::Setup() const {
    {
        NYT::NApi::TCreateNodeOptions createNodeOptions;
        createNodeOptions.Recursive = true;
        createNodeOptions.IgnoreExisting = true;
        Y_UNUSED(NYT::NConcurrency::WaitFor(
            Client->CreateNode(
                YtDirPath,
                NYT::NObjectClient::EObjectType::MapNode,
                createNodeOptions
            )
        ));
    }
    NYT::NApi::ITransactionPtr tx = NYT::NConcurrency::WaitFor(
        Client->StartTransaction(NYT::NProtoApi::ETransactionType::Master)).ValueOrThrow();
    try {
        NYT::NApi::TLockNodeOptions lockOptions;
        lockOptions.ChildKey = TableName + "_lock";
        NYT::WaitAndLock(
            tx,
            YtDirPath,
            NYT::NCypressClient::ELockMode::Shared,
            TDuration::Minutes(1),
            lockOptions
        );
        NYT::NProtoApi::TTable<NProto::TStateRow>(NJupiter::JoinYtPath(YtDirPath, TableName), Client).Setup();
    } catch (yexception& e) {
        tx->Abort();
        throw;
    }
    NYT::NConcurrency::WaitFor(tx->Commit()).ValueOrThrow();
}

int TYtStorage::CountUpdates() {
    std::shared_lock lock(Mutex);
    return static_cast<int>(Updates.size());
}

void TYtStorage::AddUpdate(const TString &key, const TString &value) {
    std::unique_lock lock(Mutex);
    Updates[key] = value;
}

static bool IsRecoverableYtError(const NYT::TErrorException& e) {
    TString errorMessage(e.what());
    bool knownError = (
        errorMessage.StartsWith("Request acknowledgment failed")
        || errorMessage.StartsWith("Request timed out")
        || errorMessage.StartsWith("Proxy cannot synchronize with cluster")
        || errorMessage.StartsWith("Request authentication failed")
        || errorMessage.StartsWith("Error committing transaction")
        || errorMessage.StartsWith("Error getting user info for user")
    );
    return knownError;
}

bool TYtStorage::ExecuteRequest(std::function<void()> request) {
    if (Status.Disabled()) {
        ERRORLOG("YT requests are disabled temporarily due to connectivity issues");
        return false;
    }
    try {
        request();
        Status.EnableIfNeeded();
    } catch (const NYT::TErrorException& e) {
        if (IsRecoverableYtError(e)) {
            Status.Disable(e.what());
            return false;
        }
        ERRORLOG("Unrecognized TErrorException during YT request: " << e.what());
        throw e;
    }
    return true;
}

bool TYtStorage::WriteRow(const TString &key, const TString &value) {
    return ExecuteRequest([&] {
        NProto::TStateRow row;
        row.SetName(key);
        row.SetState(value);
        YtStateTable.WriteRows(std::initializer_list<NProto::TStateRow>{row});
    });
}

bool TYtStorage::Flush() {
    int updates = CountUpdates();
    if (updates) {
        DEBUGLOG("Flushing " << updates << " state update(s) to YT storage...");
    } else {
        return true;
    }
    while (CountUpdates()) {
        std::unique_lock lock(Mutex);
        auto iter = Updates.begin();
        if (iter == Updates.end()) {
            return false;
        }
        if (WriteRow(iter->first, iter->second)) {
            Updates.erase(iter);
        } else {
            ERRORLOG("Cannot write row to YT table, state updates are not flushed yet");
            return false;
        }
    }
    return true;
}

bool TYtStorage::Save(const TString& key, const TString& value) {
    AddUpdate(key, value);
    return Flush();
}

TMaybe<TString> TYtStorage::Load(const TString& key) {
    TMaybe<TString> value;

    ExecuteRequest([&] {
        NProto::TStateRow keyRow;
        keyRow.SetName(key);
        TMaybe<NProto::TStateRow> row = YtStateTable.LookupRow(keyRow);
        if (row) {
            value = MakeMaybe(row->GetState());
        }
    });

    return value;
}

TVector<TString> SplitRecipients(const TString& str) {
    TVector<TString> recipients;
    if (!str.empty()) {
        Split(str, ";,", recipients);
    }
    return recipients;
}
