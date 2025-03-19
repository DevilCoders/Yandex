#include "config.h"
#include "queue.h"
#include <kernel/common_server/util/logging/backend_creator_helpers.h>
#include <kernel/common_server/util/algorithm/container.h>

namespace NCS {

    IPQClientConfig::TFactory::TRegistrator<NCS::TLogbrokerClientConfig> TLogbrokerClientConfig::Registrar(TLogbrokerClientConfig::GetTypeName());

    TString TLogbrokerClientConfig::GetTypeName() {
        return "logbroker";
    }

    TString TLogbrokerClientConfig::GetClassName() const {
        return GetTypeName();
    }

    NYdb::TDriverConfig TLogbrokerClientConfig::BuildDriverConfig() const {
        NYdb::TDriverConfig result;
        result.SetEndpoint(Endpoint);
        result.SetDatabase(Database);
        result.SetLog(Logger->CreateLogBackend());
        return result;
    }

    NYdb::NPersQueue::TPersQueueClientSettings TLogbrokerClientConfig::BuildPQClientConfig() const {
        NYdb::NPersQueue::TPersQueueClientSettings result;
        return result;
    }

    const TAuthConfig& TLogbrokerClientConfig::GetAuthConfig() const {
        return *AuthConfig;
    }

    void TLogbrokerClientConfig::DoInit(const TYandexConfig::Section* section) {
        const auto& directives = section->GetDirectives();

        directives.GetValue("Database", Database);
        directives.GetValue("Endpoint", Endpoint);
        directives.GetValue("InteractionTimeout", InteractionTimeout);

        auto children = section->GetAllChildren();
        {
            auto it = children.find("Read");
            if (it != children.end()) {
                ReadConfig.ConstructInPlace();
                ReadConfig->Init(it->second);
            }
        }
        {
            auto it = children.find("Write");
            if (it != children.end()) {
                WriteConfig.ConstructInPlace();
                WriteConfig->Init(it->second);
            }
        }
        AssertCorrectConfig(ReadConfig || WriteConfig, "no operations for logbroker client");
        Logger = NCommonServer::NUtil::CreateLogBackendCreator(*section, "Logger", "Log", "events");
        AuthConfig = TAuthConfig::Create(section);
    }

    void TLogbrokerClientConfig::DoToString(IOutputStream& os) const {
        os << "Database: " << Database << Endl;
        os << "Endpoint: " << Endpoint << Endl;
        os << "InteractionTimeout: " << InteractionTimeout << Endl;
        AuthConfig->ToString(os);
        if (ReadConfig) {
            os << "<Read>" << Endl;
            ReadConfig->ToString(os);
            os << "</Read>" << Endl;
        }
        if (WriteConfig) {
            os << "<Write>" << Endl;
            WriteConfig->ToString(os);
            os << "</Write>" << Endl;
        }
        NCommonServer::NUtil::LogBackendCreatorToString(Logger, "Logger", os);
    }

    THolder<IPQClient> TLogbrokerClientConfig::DoConstruct(const IPQConstructionContext& context) const {
        return MakeHolder<TLogbrokerClient>(GetClientId(), *this, context);
    }

    NYdb::NPersQueue::TReadSessionSettings TLogbrokerClientConfig::TReadConfig::BuildSessionConfig() const {
        NYdb::NPersQueue::TReadSessionSettings settings;
        for (const auto& i : Topics) {
            settings.AppendTopics(i);
        }
        settings.ConsumerName(ConsumerName);
        return settings;
    }

    void TLogbrokerClientConfig::TReadConfig::Init(const TYandexConfig::Section* section) {
        TVector<TString> topicsLocal;
        section->GetDirectives().FillArray("Topics", topicsLocal);
        Topics = MakeSet(topicsLocal);
        AssertCorrectConfig(!!section->GetDirectives().GetNonEmptyValue("ConsumerName", ConsumerName), "Empty consumer name");
        AssertCorrectConfig(Topics.size(), "no topics");
    }

    void TLogbrokerClientConfig::TReadConfig::ToString(IOutputStream& os) const {
        os << "Topics: " << JoinSeq(", ", Topics);
        os << "ConsumerName: " << ConsumerName << Endl;
    }

    NYdb::NPersQueue::TWriteSessionSettings TLogbrokerClientConfig::TWriteConfig::BuildSessionConfig() const {
        NYdb::NPersQueue::TWriteSessionSettings settings;
        settings.MessageGroupId(MessagesGroupId);
        settings.Path(Path);
        settings.Codec(Codec);
        settings.MaxInflightCount(MaxInFlight);
        return settings;
    }

    void TLogbrokerClientConfig::TWriteConfig::Init(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        AssertCorrectConfig(dir.GetNonEmptyValue("Path", Path), "empty topic for logbroker write client");
        AssertCorrectConfig(dir.GetNonEmptyValue("MessagesGroupId", MessagesGroupId), "MessagesGroupId must be set");
        dir.GetValue("Codec", Codec);
        dir.GetValue("MaxInFlight", MaxInFlight);
    }

    void TLogbrokerClientConfig::TWriteConfig::ToString(IOutputStream& os) const {
        os << "Path: " << Path << Endl;
        os << "MessagesGroupId: " << MessagesGroupId << Endl;
        os << "Codec: " << Codec << Endl;
        os << "MaxInFlight: " << MaxInFlight << Endl;
    }

}
