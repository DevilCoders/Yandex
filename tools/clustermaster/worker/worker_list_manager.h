#pragma once

#include <tools/clustermaster/common/master_list_manager.h>

#include <util/generic/hash.h>

struct TConfigMessage;

class TWorkerListManager: public TMasterListManager {
private:
    TString WorkerName;

public:
    TWorkerListManager();
    ~TWorkerListManager() override;

    void GetListForVariable(const TString& tag, TVector<TString>& output) const override;

    void ImportLists(const TConfigMessage* message);
};
