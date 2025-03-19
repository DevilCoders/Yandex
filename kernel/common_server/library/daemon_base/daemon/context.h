#pragma once

#include <kernel/common_server/library/daemon_base/actions_engine/controller_client.h>

#include <kernel/daemon/context.h>

#include <util/generic/noncopyable.h>

namespace NController {

class TDownloadContext : public TForceConfigReading<TDownloadContext>, private TNonCopyable {
public:
    using TDMOptions = TDaemonConfig::TControllerConfig::TDMOptions;

    TDownloadContext(const TDMOptions& dmOptions)
        : DMOptions(dmOptions)
        , ServiceType(dmOptions.ServiceType)
        , Deadline(Now() + dmOptions.Timeout)
    {
    }
    TDownloadContext& SetVersion(const TString& value) {
        Version = value;
        return *this;
    }
    TDownloadContext& SetServiceType(const TString& value) {
        ServiceType = value;
        return *this;
    }
    const TInstant GetDeadline() const {
        return Deadline;
    }
    NDaemonController::TControllerAgent CreateControllerAgent() const {
        return NDaemonController::TControllerAgent(DMOptions.Host, DMOptions.Port, nullptr, DMOptions.UriPrefix);
    }
    bool CheckDeadline() const {
        return Now() < Deadline;
    }
private:
    TDMOptions DMOptions;
    TString ServiceType;
    TString Version;
    TInstant Deadline = TInstant::Zero();
};

}
