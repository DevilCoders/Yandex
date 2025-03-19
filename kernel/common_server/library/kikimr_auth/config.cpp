#include "config.h"
#include <kernel/common_server/library/logging/events.h>
#include <kikimr/public/sdk/cpp/client/tvm/tvm.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <library/cpp/digest/md5/md5.h>

namespace NCS {

    TAuthConfig::TAuthConfig(EMethod method)
        : Method(method)
    {}

    THolder<TAuthConfig> TAuthConfig::Create(const TYandexConfig::Section* section) {
        const auto children = section->GetAllChildren();
        if (const auto* s = MapFindPtr(children, "Auth")) {
            EMethod method = EMethod::None;
            AssertCorrectConfig((*s)->GetDirectives().GetValue("Method", method), "Auth method must be set");
            THolder<TAuthConfig> result;
            switch (method) {
            case EMethod::None:
                result = MakeHolder<TAuthConfigNone>();
                break;
            case EMethod::OAuth:
                result = MakeHolder<TAuthConfigOAuth>();
                break;
            case EMethod::Tvm:
                result = MakeHolder<TAuthConfigTvm>();
                break;
            }
            result->Init(*s);
            return std::move(result);
        }
        else {
            return MakeHolder<TAuthConfigNone>();
        }
    }

    void TAuthConfig::ToString(IOutputStream& os) const {
        os << "<Auth>" << Endl;
        os << "Method: " << Method << Endl;
        DoToString(os);
        os << "</Auth>" << Endl;
    }

    TAuthConfigNone::TAuthConfigNone()
        : TAuthConfig(TAuthConfig::EMethod::None)
    {}


    bool TAuthConfigNone::PatchDriverConfig(NYdb::TDriverConfig& /*driverConfig*/, const ITvmManager* /*tvmManager*/) const {
        return true;
    }

    void TAuthConfigNone::Init(const TYandexConfig::Section* /*section*/) {
    }

    void TAuthConfigNone::DoToString(IOutputStream& /*os*/) const {
    }

    TAuthConfigOAuth::TAuthConfigOAuth()
        : TAuthConfig(TAuthConfig::EMethod::OAuth)
    {}

    bool TAuthConfigOAuth::PatchDriverConfig(NYdb::TDriverConfig& driverConfig, const ITvmManager* /*tvmManager*/) const {
        driverConfig.SetAuthToken(Token);
        return true;
    }

    void TAuthConfigOAuth::Init(const TYandexConfig::Section* section) {
        AssertCorrectConfig(section->GetDirectives().GetNonEmptyValue("Token", Token), "OAuth token must be set");
    }

    void TAuthConfigOAuth::DoToString(IOutputStream& os) const {
        os << "Token: " << MD5::Calc(Token) << Endl;
    }

    TAuthConfigTvm::TAuthConfigTvm()
        : TAuthConfig(TAuthConfig::EMethod::Tvm)
    {}

    bool TAuthConfigTvm::PatchDriverConfig(NYdb::TDriverConfig& driverConfig, const ITvmManager* tvmManager) const {
        if (!tvmManager) {
            TFLEventLog::Error("There is not TvmManager not found");
            return false;
        }
        if (auto tvmClient = tvmManager->GetTvmClient(TvmClientName)) {
            driverConfig.SetCredentialsProviderFactory(NYdb::CreateTVMCredentialsProviderFactory(tvmClient.Get(), DestinationAlias));
        }
        else {
            TFLEventLog::Error("TvmClientName not found")("tvm_client_name", TvmClientName);
            return false;
        }
        return true;
    }

    void TAuthConfigTvm::Init(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        AssertCorrectConfig(dir.GetNonEmptyValue("TvmClientName", TvmClientName) || dir.GetNonEmptyValue("SelfClientId", TvmClientName), "TvmClientName must be set");
        dir.GetNonEmptyValue("DestinationAlias", DestinationAlias);
    }

    void TAuthConfigTvm::DoToString(IOutputStream& os) const {
        os << "TvmClientName: " << TvmClientName << Endl;
        os << "DestinationAlias: " << DestinationAlias << Endl;
    }

}
