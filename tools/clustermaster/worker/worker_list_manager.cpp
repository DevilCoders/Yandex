#include "worker_list_manager.h"

#include "log.h"
#include "messages.h"

#include <tools/clustermaster/common/vector_to_string.h>

#include <util/generic/bt_exception.h>

TWorkerListManager::TWorkerListManager() {
}

TWorkerListManager::~TWorkerListManager() {
}

void TWorkerListManager::ImportLists(const TConfigMessage* message) {
    LOG("Importing configs received from master...");

    Reset();

    WorkerName = message->GetWorkerName();

    TVector<TString> hostCfgs;
    for (int i = 0; i < message->GetHostCfgs().size(); ++i) {
        hostCfgs.push_back(message->GetHostCfgs(i));
    }
    LoadHostcfgsFromStrings(hostCfgs);

    if (!message->GetHostList().empty()) {
        LoadHostlistFromString(message->GetHostList());
    }

    LOG("Importing configs received from master done");
}

void TWorkerListManager::GetListForVariable(const TString& tag, TVector<TString>& output) const {
    TMasterListManager::GetList(tag, WorkerName, output);
}
