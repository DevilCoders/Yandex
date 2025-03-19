#pragma once

#include "base_controller.h"

namespace {
    template<class TConfig, bool HasToString>
    struct TConfigToString {
        static TString Get(NController::TController& owner) {
            return owner.GetConfigTextUnsafe();
        }
    };

    template<class TConfig>
    struct TConfigToString<TConfig, true> {
        static TString Get(NController::TController& owner) {
            NController::TController::TGuardConfig g = owner.GetConfig();
            if (!!g)
                return g->GetMeAs<TConfig>().ToString();
            else
                return owner.GetConfigTextUnsafe();
        }
    };

    Y_HAS_MEMBER(ToString);

    template<class TBaseServer, bool HasCanSetConfig>
    struct TCanSetConfig {
        static bool Get(NController::TController::TGuardServer& /*gs*/) {
            return true;
        }
    };

    template<class TBaseServer>
    struct TCanSetConfig <TBaseServer, true> {
        static bool Get(NController::TController::TGuardServer& gs) {
            return gs->GetLogicServer().GetMeAs<TBaseServer>().CanSetConfig();
        }
    };
    Y_HAS_MEMBER(CanSetConfig);
}

template <class TBaseServer>
class TServerDescriptor : public NController::TController::IServerDescriptor {
public:
    typedef typename TBaseServer::TConfig TConfig;
    typedef typename TBaseServer::TInfoCollector TInfoCollector;
    typedef NObjectFactory::TParametrizedObjectFactory<NController::TController::TCommandProcessor, TString, TBaseServer*> TCommandFactory;

    virtual IServerConfig* CreateConfig(const TServerConfigConstructorParams& constParams) const override {
        return IServerConfig::BuildConfig<TConfig>(constParams).Release();
    }

    virtual NController::IServer* CreateServer(const IServerConfig& config) const override {
        return new TBaseServer(config.GetMeAs<TConfig>());
    }

    virtual TCollectServerInfo* CreateInfoCollector() const override {
        return new TInfoCollector;
    }

    virtual NController::TController::TCommandProcessor* CreateCommandProcessor(const TString& command) const override {
        NController::TController::TCommandProcessor* result = NController::TController::TCommandProcessor::TFactory::Construct(command);
        if (!result)
            result = TCommandFactory::Construct(command, nullptr);
        return result;
    }

    virtual TString GetConfigString(NController::TController& owner) const override {
        return TConfigToString<TConfig, THasToString<TConfig>::value>::Get(owner);
    }

    virtual bool CanSetConfig(NController::TController::TGuardServer& gs) const override {
        return TCanSetConfig<TBaseServer, THasCanSetConfig<TBaseServer>::value>::Get(gs);
    }
};
