#pragma once

#include <kernel/common_server/processors/common/handler.h>


namespace NServiceMonitor {
    class TControllerCommandHandler: public TRequestHandlerBase<TControllerCommandHandler> {
        using TBase = TRequestHandlerBase<TControllerCommandHandler>;
        using TConfig = TCommonRequestHandlerConfig<TControllerCommandHandler, TEmptyConfig>;

    public:
        TControllerCommandHandler(const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server);
        void ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) override final;
        static TString GetTypeName();

    private:
        using TBase::Config;
    };
}
