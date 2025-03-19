#pragma once
#include <kernel/common_server/roles/abstract/manager.h>
#include <kernel/common_server/api/links/manager.h>

class TDBItemsManagerConfig {
private:
    CSA_READONLY_DEF(TDBEntitiesManagerConfig, ManagerConfig);
public:
    void Init(const TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        auto it = children.find("ManagerConfig");
        if (it != children.end()) {
            ManagerConfig.Init(it->second);
        }
    }
    void ToString(IOutputStream& os) const {
        os << "<ManagerConfig>" << Endl;
        ManagerConfig.ToString(os);
        os << "</ManagerConfig>" << Endl;
    }
};

class TDBRolesManagerConfig {
private:
    CSA_READONLY_DEF(TDBEntitiesManagerConfig, ManagerConfig);
public:
    void Init(const TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        auto it = children.find("ManagerConfig");
        if (it != children.end()) {
            ManagerConfig.Init(it->second);
        }
    }
    void ToString(IOutputStream& os) const {
        os << "<ManagerConfig>" << Endl;
        ManagerConfig.ToString(os);
        os << "</ManagerConfig>" << Endl;
    }
};

class TDBFullRolesManagerConfig: public IRolesManagerConfig {
private:
    static TFactory::TRegistrator<TDBFullRolesManagerConfig> Registrator;
    CSA_DEFAULT(TDBFullRolesManagerConfig, TDBItemsManagerConfig, ItemsManagerConfig);
    CSA_DEFAULT(TDBFullRolesManagerConfig, TDBRolesManagerConfig, RolesManagerConfig);
    CSA_DEFAULT(TDBFullRolesManagerConfig, TLinkManagerConfig, RoleRoleLinksManagerConfig);
    CSA_DEFAULT(TDBFullRolesManagerConfig, TLinkManagerConfig, RoleItemLinksManagerConfig);
    CSA_DEFAULT(TDBFullRolesManagerConfig, TString, DBName);
protected:
    virtual void DoInit(const TYandexConfig::Section* section) override {
        auto children = section->GetAllChildren();
        DBName = section->GetDirectives().Value("DBName", DBName);
        {
            auto it = children.find("ItemsManagerConfig");
            if (it != children.end()) {
                ItemsManagerConfig.Init(it->second);
            }
        }
        {
            auto it = children.find("RolesManagerConfig");
            if (it != children.end()) {
                RolesManagerConfig.Init(it->second);
            }
        }
        {
            auto it = children.find("RoleRoleLinksManagerConfig");
            if (it != children.end()) {
                RoleRoleLinksManagerConfig.Init(it->second);
            }
        }
        {
            auto it = children.find("RoleItemLinksManagerConfig");
            if (it != children.end()) {
                RoleItemLinksManagerConfig.Init(it->second);
            }
        }
    }
    virtual void DoToString(IOutputStream& os) const override {
        os << "<ItemsManagerConfig>" << Endl;
        ItemsManagerConfig.ToString(os);
        os << "</ItemsManagerConfig>" << Endl;
        os << "<RolesManagerConfig>" << Endl;
        RolesManagerConfig.ToString(os);
        os << "</RolesManagerConfig>" << Endl;
        os << "<RoleRoleLinksManagerConfig>" << Endl;
        RoleRoleLinksManagerConfig.ToString(os);
        os << "</RoleRoleLinksManagerConfig>" << Endl;
        os << "<RoleItemLinksManagerConfig>" << Endl;
        RoleItemLinksManagerConfig.ToString(os);
        os << "</RoleItemLinksManagerConfig>" << Endl;
    }
public:
    virtual THolder<IRolesManager> BuildManager(const IBaseServer& server) const override;
    static TString GetTypeName() {
        return "db";
    }

    virtual TString GetClassName() const override {
        return GetTypeName();
    }
};
