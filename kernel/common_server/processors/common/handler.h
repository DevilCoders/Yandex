#pragma once

#include "config.h"
#include <kernel/common_server/auth/common/auth.h>
#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>
#include <kernel/common_server/roles/actions/common.h>
#include <kernel/common_server/user_role/abstract/abstract.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>
#include <kernel/common_server/util/algorithm/ptr.h>

class TBaseServer;

class TCommonRequestHandler: public TAuthRequestProcessor {
    using TBase = TAuthRequestProcessor;

public:
    using TBase::TBase;
    virtual void DoAuthProcess(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo);

protected:
    virtual void ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) = 0;
};

template <class TBaseHandler, class TConfigImpl = TEmptyConfig>
class TRequestHandlerBase : public TCommonRequestHandler {
protected:
    using THandlerConfig = TCommonRequestHandlerConfig<TBaseHandler, TConfigImpl>;
    const THandlerConfig& Config;
private:
    const THandlerConfig Registrator = { "dummy" };

public:
    TRequestHandlerBase(const THandlerConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
        : TCommonRequestHandler(config, context, authModule, server)
        , Config(config)
    {
        DEBUG_LOG << Config.Registrator.GetName() << " registered" << Endl;
    }
};

template <class TPermissions, class TServer, class TConfigImpl = TEmptyConfig>
class TRequestWithPermissions: public TCommonRequestHandler {
    using THandlerConfig = TBaseAuthConfig<TConfigImpl>;
protected:
    const THandlerConfig& Config;

    const TServer& GetServer() const {
        return TCommonRequestHandler::GetServer<TServer>();
    }

    template <class T, class... TArgs>
    void ReqCheckPermissions(typename TPermissions::TPtr permissions, TArgs... args) {
        ReqCheckCondition(permissions->template Check<T>(args...),
            ConfigHttpStatus.PermissionDeniedStatus, ELocalizationCodes::NoPermissions);
    }

    virtual void ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) override final {
        auto evLogContext = TFLRecords::StartContext();
        ReqCheckCondition(!!authInfo, HTTP_INTERNAL_SERVER_ERROR, "authorization.problems.incorrect_info");
        TString internalUserId;
        ReqCheckCondition(BaseServer->GetAuthUsersManager().AuthUserIdToInternalUserId(authInfo->GetAuthId(), internalUserId), HTTP_INTERNAL_SERVER_ERROR, "permissions.problems.cannot_restore_user");
        IPermissionsManager::TGetPermissionsContext gpContext(internalUserId, authInfo->GetModuleName());
        typename TPermissions::TPtr permissions = std::dynamic_pointer_cast<TPermissions>(BaseServer->GetPermissionsManager().GetPermissions(gpContext));
        ReqCheckCondition(!!permissions, HTTP_INTERNAL_SERVER_ERROR, "permissions.problems.cannot_restore");
        evLogContext("user_id", permissions->GetUserId());
        if (GetCgiParameters().Has("dump", "eventlog")) {
            TFLEventLog::Log("eventlog dumping on");
        }
        g.SetCode(HTTP_OK);
        ProcessRequestWithPermissions(g, permissions);
    }

    virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, typename TPermissions::TPtr permissions) = 0;
public:
    TRequestWithPermissions(const THandlerConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
        : TCommonRequestHandler(config, context, authModule, server)
        , Config(config)
    {
    }
};

template <class TProductClass, class TBaseClass, class TConfigImpl = TEmptyConfig>
class TRequestWithRegistrator: public TBaseClass {
public:
    using TRegistrationHandlerDefaultConfig = TCommonRequestHandlerConfig<TProductClass, TConfigImpl>;
private:
    const TRegistrationHandlerDefaultConfig Registrator = { "dummy" };
public:
    TRequestWithRegistrator(const TRegistrationHandlerDefaultConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
        : TBaseClass(config, context, authModule, server) {
        DEBUG_LOG << config.Registrator.GetName() << " registered" << Endl;
    }
};

template <class TProductClass, class TConfigImpl = TEmptyConfig>
using TCommonSystemHandler = TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer, TConfigImpl>, TConfigImpl>;

namespace NCS {
    namespace NHandlers {
        template <class TProductClass, class TUserPermissions, class TServer>
        class TCommonImpl: public TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TUserPermissions, TServer>> {
        private:
            using TBase = TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TUserPermissions, TServer>>;
        protected:
            using TBase::ReqCheckCondition;
            using TBase::ConfigHttpStatus;
        public:
            using TBase::TBase;
        };

        template <class TProductClass, class TObjectInfo, class TUserPermissions, class TServer>
        class TCommonInfo: public TCommonImpl<TProductClass, TUserPermissions, TServer> {
        private:
            using TBase = TCommonImpl<TProductClass, TUserPermissions, TServer>;
        protected:
            virtual bool DoFillHandlerScheme(NCS::NScheme::THandlerScheme& scheme, const IBaseServer& server) const override {
                auto& rMethod = scheme.Method(NCS::NScheme::ERequestMethod::Post);
                auto& content = rMethod.Body().Content();
                content.Add<TFSArray>("objects").SetElement<TFSString>().SetRequired(false);
                auto& replyScheme = rMethod.Response(HTTP_OK).AddContent();
                replyScheme.Add<TFSArray>("objects").SetElement(TObjectInfo::GetScheme(server.GetAsSafe<TServer>()));
                return true;
            }
        public:
            using TBase::TBase;
        };

        template <class TProductClass, class TObjectInfo, class TUserPermissions, class TServer>
        class TCommonUpsert: public TCommonImpl<TProductClass, TUserPermissions, TServer> {
        private:
            using TBase = TCommonImpl<TProductClass, TUserPermissions, TServer>;
        protected:
            virtual bool DoFillHandlerScheme(NCS::NScheme::THandlerScheme& scheme, const IBaseServer& server) const override {
                auto& rMethod = scheme.Method(NCS::NScheme::ERequestMethod::Post);
                auto& content = rMethod.Body().Content();
                content.Add<TFSArray>("objects").SetElement<TFSStructure>(TObjectInfo::GetScheme(server.GetAsSafe<TServer>())).SetRequired(true);
                rMethod.Response(HTTP_OK).AddContent();
                return true;
            }
        public:
            using TBase::TBase;
        };

        template <class TProductClass, class TObjectInfo, class TUserPermissions, class TServer>
        class TCommonRemove: public TCommonImpl<TProductClass, TUserPermissions, TServer> {
        private:
            using TBase = TCommonImpl<TProductClass, TUserPermissions, TServer>;
        protected:
            virtual bool DoFillHandlerScheme(NCS::NScheme::THandlerScheme& scheme, const IBaseServer& /*server*/) const override {
                auto& rMethod = scheme.Method(NCS::NScheme::ERequestMethod::Post);
                auto& content = rMethod.Body().Content();
                content.Add<TFSArray>("objects").SetElement<TFSString>().SetRequired(true);
                rMethod.Response(HTTP_OK).AddContent();
                return true;
            }
        public:
            using TBase::TBase;
        };
    }
}
