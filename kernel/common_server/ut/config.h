#pragma once

#include "const.h"

#include <kernel/common_server/util/accessor.h>

#include <kernel/daemon/config/config_constructor.h>
#include <kernel/daemon/config/daemon_config.h>

#include <library/cpp/logger/global/global.h>

#include <util/stream/str.h>
#include <util/string/subst.h>
#include <util/string/cast.h>


namespace NServerTest {

    class TConfigGenerator {
        CS_ACCESS(TConfigGenerator, ui64, LogLevel, 7);
        CS_ACCESS(TConfigGenerator, ui64, DaemonPort, 0);
        CS_ACCESS(TConfigGenerator, ui64, BasePort, 0);
        CS_ACCESS(TConfigGenerator, ui64, MonitoringPort, 1111);
        CS_ACCESS(TConfigGenerator, ui64, DBPort, 6432);

        CSA_DEFAULT(TConfigGenerator, TString, HomeDir);
        CSA_DEFAULT(TConfigGenerator, TString, WorkDir);

        CSA_DEFAULT(TConfigGenerator, TString, PPass);
        CSA_DEFAULT(TConfigGenerator, TString, DBHost);
        CSA_DEFAULT(TConfigGenerator, TString, DBName);
        CSA_DEFAULT(TConfigGenerator, TString, DBUser);
        CSA_DEFAULT(TConfigGenerator, TString, DBMainNamespace);
    protected:
        TString BuildMigrationsFromFolders(const TString& folders) const;
        virtual TString GetExternalDatabasesConfiguration() const;
        virtual TString GetExternalQueuesConfiguration() const {
            return "";
        }

        virtual TString GetExternalApiConfiguration() const {
            return TString("");
        }

        virtual TString GetCustomManagersConfiguration() const {
            return TString("");
        }

        virtual TString GetProcessorsConfiguration() const {
            return TString("");
        }

        virtual TString GetAuthConfiguration() const {
            TStringStream ss;
            ss << "<fake>" << Endl;
            ss << "Type: fake" << Endl;
            ss << "</fake>" << Endl;
            return ss.Str();
        }
    public:
        virtual ~TConfigGenerator() {}

        TString GetFullConfig() const;

        template<class TConfigImpl>
        static THolder <TConfigImpl> BuildConfig(const TString& configStr) {
            THolder <TConfigImpl> config;
            TServerConfigConstructorParams params(configStr.data());

            auto rawConfig = MakeHolder<TAnyYandexConfig>();
            CHECK_WITH_LOG(rawConfig->ParseMemory(configStr));
            config = MakeHolder<TConfigImpl>(params);

            auto children = rawConfig->GetRootSection()->GetAllChildren();
            {
                auto it = children.find("Server");
                if (it != children.end()) {
                    config->Init(it->second);
                }
            }
            return config;
        }

    private:
        TString PatchConfiguration(const TString& input) const;
    };
}
