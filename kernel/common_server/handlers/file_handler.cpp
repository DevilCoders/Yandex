#include "file_handler.h"

#include <library/cpp/resource/resource.h>

#include <util/stream/file.h>

namespace NLogistics {
    bool TFileHandlerConfig::InitFeatures(const TYandexConfig::Section* section) {
        FilePath = section->GetDirectives().Value("FilePath", FilePath);
        ResourceKey = section->GetDirectives().Value("ResourceKey", ResourceKey);
        ContentType = section->GetDirectives().Value("ContentType", ContentType);
        return true;
    }

    void TFileHandlerConfig::ToStringFeatures(IOutputStream& os) const {
        os << "FilePath: " << FilePath << Endl;
        os << "ResourceKey: " << ResourceKey << Endl;
        os << "ContentType: " << ContentType << Endl;
    }

    TFileHandler::TFileHandler(const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
        : TBase(config, context, authModule, server)
        , Config(config)
    {
    }

    void TFileHandler::ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) {
        Y_UNUSED(authInfo);

        TString content;
        if (Config.FilePath) {
            TMappedFileInput in(Config.FilePath);
            content = in.ReadAll();
        } else {
            content = NResource::Find(Config.ResourceKey);
        }

        g.MutableReport().SetOverridenContentType(Config.ContentType);
        g.SetExternalReportString(std::move(content), false);
        g.SetCode(HTTP_OK);
    }
}
