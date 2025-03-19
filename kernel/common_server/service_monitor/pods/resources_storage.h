#pragma once

#include "abstract.h"

#include <kernel/common_server/service_monitor/services/yp.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/service_monitor/pods/errors.h>
#include <yp/cpp/yp/client.h>


namespace NServiceMonitor {

    class TServerConfig;

    class TYpResourcesInfoStorage: public IResourcesInfoStorage {
    public:
        TYpResourcesInfoStorage(const TYpServicesOperator& ypServices);

        TVector<NProto::TResourceRequests> GetResourcesInfo(const TVector<TPodLocator>& podLocators) const override;

    private:
        const TYpServicesOperator& YpServices;
    };
}
