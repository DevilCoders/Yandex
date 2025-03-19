#pragma once

#include <kernel/common_server/processors/common/handler.h>

namespace NLogistics {
    class TFileHandlerConfig {
    public:
        TString FilePath;
        TString ResourceKey;
        TString ContentType;

        bool InitFeatures(const TYandexConfig::Section* section);

        void ToStringFeatures(IOutputStream& os) const;
    };

    class TFileHandler: public TRequestHandlerBase<TFileHandler, TFileHandlerConfig> {
        using TBase = TRequestHandlerBase<TFileHandler, TFileHandlerConfig>;
        using TConfig = TCommonRequestHandlerConfig<TFileHandler, TFileHandlerConfig>;

    public:
        TFileHandler(const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server);

        void ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) override final;

        static TString GetTypeName() {
            return "file";
        }

    private:
        const TFileHandlerConfig Config;
    };
}
