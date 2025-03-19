#include "starter.h"

#include <cloud/blockstore/libs/daemon/app.h>
#include <cloud/blockstore/libs/service/device_handler.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/blockstore/libs/service/storage_test.h>

#include <cloud/storage/core/libs/common/app.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/common/network.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/network/sock.h>
#include <util/random/random.h>
#include <util/system/backtrace.h>
#include <util/system/getpid.h>

#include <fstream>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TDeviceHandlerFactoryWithHolder
    : public IDeviceHandlerFactory
{
private:
    IDeviceHandlerFactoryPtr DefaultFactory;
    NFuzzing::TStarter::DevicesVector Devices;

public:
    TDeviceHandlerFactoryWithHolder(IDeviceHandlerFactoryPtr DefaultFactory)
        : DefaultFactory(std::move(DefaultFactory))
    {}

    IDeviceHandlerPtr CreateDeviceHandler(
        IStoragePtr storage,
        TString clientId,
        ui32 blockSize,
        ui32 zeroBlocksCountLimit,
        bool unalignedRequestsDisabled,
        NMonitoring::TDynamicCounters::TCounterPtr unalignedRequestCounter) \
            override final
    {
        IDeviceHandlerPtr device = DefaultFactory->CreateDeviceHandler(
            storage,
            std::move(clientId),
            blockSize,
            zeroBlocksCountLimit,
            unalignedRequestsDisabled,
            std::move(unalignedRequestCounter));
        Devices.emplace_back(device);
        return device;
    }

    NFuzzing::TStarter::DevicesVector GetDevices()
    {
        return Devices;
    }
};

NServer::TOptions::EServiceKind GetServiceKind()
{
    const TString serviceKind =
        GetTestParam("nbs_service_kind", "null");

    return serviceKind == "kikimr"
           ? NServer::TOptions::EServiceKind::Kikimr
           : serviceKind == "local"
             ? NServer::TOptions::EServiceKind::Local
             : NServer::TOptions::EServiceKind::Null;
}

}   // namespace

}   // namespace NCloud::NBlockStore

namespace NCloud::NBlockStore::NFuzzing {

////////////////////////////////////////////////////////////////////////////////

using namespace NCloud::NBlockStore::NServer;

/*static*/ TStarter* TStarter::GetStarter()
{
    static TVector<NTesting::TPortHolder> ports;
    static std::unique_ptr<TStarter> Impl = nullptr;

    if (!Impl) {
        ConfigureSignals();

        TString kikimr_port;
        std::ifstream("testing_out_stuff/nbs_configs/kikimr_port.txt")
            >> kikimr_port;

        auto options = std::make_shared<TOptions>();
        options->Domain =
            "Root";
        options->DynamicNameServiceConfig =
            "testing_out_stuff/nbs_configs/dyn_ns.txt";
        options->DomainsConfig =
            "testing_out_stuff/nbs_configs/domains.txt";
        options->FeaturesConfig =
            "testing_out_stuff/nbs_configs/features.txt";
        options->LogConfig =
            "testing_out_stuff/nbs_configs/server-log.txt";
        options->SysConfig =
            "testing_out_stuff/nbs_configs/server-sys.txt";
        options->ServerConfig =
            "testing_out_stuff/nbs_configs/server.txt";
        options->StorageConfig =
            "testing_out_stuff/nbs_configs/storage.txt";
        options->DiagnosticsConfig =
            "testing_out_stuff/nbs_configs/diag.txt";
        options->DiskRegistryProxyConfig =
            "testing_out_stuff/nbs_configs/dr_proxy.txt";
        options->ServiceKind =
            GetServiceKind();
        options->SchemeShardDir =
            "nbs";
        options->LoadCmsConfigs =
            true;
        options->NodeBrokerAddress =
            "localhost:" + kikimr_port;
        options->MonitoringPort =
            *ports.insert(ports.end(), NTesting::GetFreePort());
        options->InterconnectPort =
            *ports.insert(ports.end(), NTesting::GetFreePort());
        options->DataServerPort =
            *ports.insert(ports.end(), NTesting::GetFreePort());
        options->ServerPort =
            *ports.insert(ports.end(), NTesting::GetFreePort());

        Impl = std::unique_ptr<TStarter>(
            new TStarter(std::move(options)));

        std::set_terminate([]{
            TBackTrace bt;
            bt.Capture();
            Cerr << bt.PrintToString() << Endl;
            abort();
        });

        Impl->Start();

        atexit([]()
        {
            TStarter::GetStarter()->Stop();
        });
    }

    return Impl.get();
}

TStarter::TStarter(TOptionsPtr options)
    : DeviceHandlerFactory(std::make_shared<TDeviceHandlerFactoryWithHolder>(
          CreateDefaultDeviceHandlerFactory()))
    , Bootstrap(std::move(options))
{
    TLogSettings logSettings;
    logSettings.UseLocalTimestamps = true;

    FuzzerLogging = CreateLoggingService("console", logSettings);
    Log = FuzzerLogging->CreateLog("BLOCKSTORE_FUZZER");
}

void TStarter::Start()
{
    try {
        Bootstrap.Init(DeviceHandlerFactory);
        Bootstrap.Start();
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        Bootstrap.Stop();
        throw;
    }

    Thread = SystemThreadFactory()->Run([this](){
        AppMain(Bootstrap.GetShouldContinue());
    });

    {
        auto request = std::make_shared<NProto::TCreateVolumeRequest>();
        request->SetDiskId("vol_" + ToString(GetPID()));
        request->SetBlocksCount(100000000);
        request->SetBlockSize(4096);
        request->SetStorageMediaKind(::NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_HDD);

        auto future = Bootstrap.GetBlockStoreService()->CreateVolume(
            MakeIntrusive<TCallContext>(),
            std::move(request));
        future.GetValueSync();
    }

    {
        auto request = std::make_shared<NProto::TStartEndpointRequest>();
        request->SetDiskId("vol_" + ToString(GetPID()));
        request->SetIpcType(NProto::IPC_VHOST);
        request->SetClientId("fuzzer");
        request->SetUnixSocketPath("server_socket_" + ToString(GetPID()));

        auto future = Bootstrap.GetBlockStoreService()->StartEndpoint(
            MakeIntrusive<TCallContext>(),
            std::move(request));
        future.GetValueSync();
    }
}

void TStarter::Stop()
{
    AppStop(0);
    Thread->Join();
    try {
        Bootstrap.Stop();
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        throw;
    }
}

TLog& TStarter::GetLogger()
{
    return Log;
}

TStarter::DevicesVector TStarter::GetDevices()
{
    return static_cast<TDeviceHandlerFactoryWithHolder*>(
        DeviceHandlerFactory.get())->GetDevices();
}

}   // namespace NCloud::NBlockStore::NFuzzing
