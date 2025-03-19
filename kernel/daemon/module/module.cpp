#include "module.h"

TDaemonModules::TDaemonModules(const IServerConfig& config)
    : Config(config)
{
    const TSet<TString> objectNames = Config.GetModulesSet();
    for (auto i : objectNames) {
        THolder<IDaemonModule> product;
        if (!product) {
            product = THolder<IDaemonModule>(TFactory::Construct(i, Config));
        }
        if (!product) {
            product = THolder<IDaemonModule>(IDaemonModule::TFactory::Construct(i, Config.GetDaemonConfig()));
        }
        VERIFY_WITH_LOG(product, "Incorrect module name: %s", i.data());
        Modules.push_back(std::move(product));
    }
}

bool TDaemonModules::Start() {
    for (ui32 module = 0; module < Modules.size(); ++module) {
        INFO_LOG << "Module " << Modules[module]->Name() << " starting" << Endl;
        VERIFY_WITH_LOG(Modules[module]->Start(), "incorrect start method result for %s module", Modules[module]->Name().data());
        INFO_LOG << "Module " << Modules[module]->Name() << " started" << Endl;
    }
    return true;
}

bool TDaemonModules::Stop(const IDaemonModule::TStopContext& stopContext) {
    for (ui32 module = 0; module < Modules.size(); ++module) {
        VERIFY_WITH_LOG(Modules[module]->Stop(stopContext), "incorrect stop method result for %s module", Modules[module]->Name().data());
    }
    return true;
}
