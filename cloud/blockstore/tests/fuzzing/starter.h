#pragma once

#include <cloud/blockstore/libs/daemon/bootstrap.h>
#include <cloud/blockstore/libs/daemon/options.h>
#include <cloud/storage/core/libs/common/startable.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/thread/factory.h>


namespace NCloud::NBlockStore::NFuzzing {

////////////////////////////////////////////////////////////////////////////////

class TStarter
    : public IStartable
{
private:
    ILoggingServicePtr FuzzerLogging;
    TLog Log;

    THolder<IThreadFactory::IThread> Thread;
    IDeviceHandlerFactoryPtr DeviceHandlerFactory;
    NServer::TBootstrap Bootstrap;

public:
    using DevicesVector = TVector<IDeviceHandlerPtr>;

    static TStarter* GetStarter();

    void Start() override;
    void Stop() override;

    TLog& GetLogger();
    DevicesVector GetDevices();

private:
    TStarter(NServer::TOptionsPtr options);

};

}   // namespace NCloud::NBlockStore::NFuzzing
