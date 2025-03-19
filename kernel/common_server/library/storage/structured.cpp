#include "structured.h"

#include <kernel/common_server/util/algorithm/iterator.h>
#include <kernel/common_server/util/algorithm/container.h>

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/logger/global/global.h>

#include <util/string/join.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <util/system/hostname.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>

namespace {
    bool DBEventLogGuardActiveFlag = true;
}

void TDBEventLogGuard::SetActive(const bool activeFlag) {
    DBEventLogGuardActiveFlag = activeFlag;
}

TDBEventLogGuard::TDBEventLogGuard(NStorage::IDatabase::TPtr database, const TString& eventDescription, const TString& tableName)
    : Database(database)
    , EventDescription(eventDescription)
    , TableName(tableName)
    , Active(DBEventLogGuardActiveFlag)
{
    if (!Active) {
        return;
    }
    if (SendGlobalMessage(CDIMessage)) {
        auto table = Database->GetTable(TableName);
        NStorage::TTableRecord tr;
        tr.Set("ctype", CDIMessage.GetCType());
        tr.Set("service", CDIMessage.GetService());
        tr.Set("revision", GetProgramSvnRevision());
        tr.Set("host", GetHostName());
        tr.Set("event", EventDescription + " START");
        tr.Set("instant", Start.MicroSeconds());
        auto transaction = Database->CreateTransaction(false);
        auto result = table->AddRow(tr, transaction);
        if (!result || !result->IsSucceed() || !transaction->Commit()) {
            ERROR_LOG << transaction->GetStringReport() << Endl;
        }
    }
}

TDBEventLogGuard::~TDBEventLogGuard() {
    if (!Active) {
        return;
    }
    if (CDIMessage.Initialized()) {
        auto table = Database->GetTable(TableName);
        NStorage::TTableRecord tr;
        tr.Set("ctype", CDIMessage.GetCType());
        tr.Set("service", CDIMessage.GetService());
        tr.Set("revision", GetProgramSvnRevision());
        tr.Set("host", GetHostName());
        tr.Set("event", EventDescription + " FINISHED");
        tr.Set("instant", Now().MicroSeconds());
        tr.Set("duration", (Now() - Start).MicroSeconds());
        auto transaction = Database->CreateTransaction(false);
        auto result = table->AddRow(tr, transaction);
        if (!result || !result->IsSucceed() || !transaction->Commit()) {
            ERROR_LOG << transaction->GetStringReport() << Endl;
        }
    }
}
