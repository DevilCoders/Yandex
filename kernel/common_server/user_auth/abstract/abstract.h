#pragma once
#include <kernel/common_server/library/interfaces/container.h>
#include <library/cpp/yconf/conf.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/abstract/frontend.h>

class TAuthUserLinkId {
private:
    CSA_PROTECTED_DEF(TAuthUserLinkId, TString, AuthModuleId);
    CSA_PROTECTED_DEF(TAuthUserLinkId, TString, AuthUserId);
public:
    TAuthUserLinkId() = default;
    TAuthUserLinkId(const TString& authModuleId, const TString& authUserId)
        : AuthModuleId(authModuleId)
        , AuthUserId(authUserId)
    {

    }

    static NFrontend::TScheme GetScheme(const IBaseServer& server);

    NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    TString GetAuthIdString(const bool useAuthModuleId = true) const;
};

class TAuthUserLink: public TAuthUserLinkId {
private:
    using TBase = TAuthUserLinkId;
    CSA_PROTECTED(TAuthUserLink, ui32, LinkId, 0);
    CSA_PROTECTED_DEF(TAuthUserLink, TString, SystemUserId);
public:
    TAuthUserLink() = default;

    TAuthUserLink(const TAuthUserLinkId& base)
        : TBase(base)
    {

    }

    NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    static NFrontend::TScheme GetScheme(const IBaseServer& server);
    static NFrontend::TScheme GetSearchScheme(const IBaseServer& server);
};

class IAuthUsersManager {
protected:
    const IBaseServer& Server;
    virtual bool DoAuthUserIdToInternalUserId(const TAuthUserLinkId& authId, TString& internalUserId) const = 0;
public:
    IAuthUsersManager(const IBaseServer& server)
        : Server(server)
    {

    }

    using TPtr = TAtomicSharedPtr<IAuthUsersManager>;
    virtual ~IAuthUsersManager() = default;
    virtual bool AuthUserIdToInternalUserId(const TAuthUserLinkId& authId, TString& internalUserId) const;
    virtual bool Upsert(const TAuthUserLink& userData, const TString& userId, bool* isUpdate) const;
    virtual bool Remove(const TVector<TAuthUserLinkId>& authUserIds, const TString& userId) const;
    virtual bool Remove(const TVector<ui32>& linkIds, const TString& userId) const;
    virtual bool Restore(const TAuthUserLinkId& authUserId, TMaybe<TAuthUserLink>& data) const = 0;
    virtual bool Restore(const TString& systemUserId, TVector<TAuthUserLink>& data) const = 0;
    virtual bool StartManager() = 0;
    virtual bool StopManager() = 0;

    virtual bool RestoreAll(TVector<TAuthUserLink>& data) const;
};

class IAuthUsersManagerConfig {
public:
    using TPtr = TAtomicSharedPtr<IAuthUsersManagerConfig>;
    using TFactory = NObjectFactory::TObjectFactory<IAuthUsersManagerConfig, TString>;
    virtual ~IAuthUsersManagerConfig() = default;
    virtual IAuthUsersManager::TPtr BuildManager(const IBaseServer& server) const = 0;
    virtual void Init(const TYandexConfig::Section* section) = 0;
    virtual void ToString(IOutputStream& os) const = 0;
    virtual TString GetClassName() const = 0;
};

class TAuthUsersManagerConfig: public TBaseInterfaceContainer<IAuthUsersManagerConfig> {
private:
    using TBase = TBaseInterfaceContainer<IAuthUsersManagerConfig>;
public:

};
