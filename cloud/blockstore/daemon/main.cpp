#include <cloud/blockstore/libs/daemon/app.h>
#include <cloud/blockstore/libs/daemon/bootstrap.h>
#include <cloud/blockstore/libs/daemon/options.h>
#include <cloud/blockstore/libs/service/device_handler.h>

#include <cloud/storage/core/libs/common/app.h>

#include <util/generic/yexception.h>

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    using namespace NCloud::NBlockStore::NServer;

    ConfigureSignals();

    auto options = std::make_shared<TOptions>();
    try {
        options->Parse(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }

    TBootstrap bootstrap(std::move(options));
    try {
        bootstrap.Init(NCloud::NBlockStore::CreateDefaultDeviceHandlerFactory());
        bootstrap.Start();
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        bootstrap.Stop();
        return 1;
    }

    int exitCode = NCloud::AppMain(bootstrap.GetShouldContinue());

    try {
        bootstrap.Stop();
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }

    return exitCode;
}
