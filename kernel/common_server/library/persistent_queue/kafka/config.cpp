#include "config.h"
#include "queue.h"
#include <kernel/common_server/util/logging/backend_creator_helpers.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/logger/init_context/yconf.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <util/generic/algorithm.h>

namespace NCS {

    TString TKafkaConfig::GetTypeName() {
        return "kafka";
    }

    TString TKafkaConfig::GetClassName() const {
        return GetTypeName();
    }

    TKafkaConfig::TFactory::TRegistrator<TKafkaConfig> TKafkaConfig::Registrator(TKafkaConfig::GetTypeName());

    cppkafka::Configuration TKafkaConfig::BuildKafkaConfig() const {
        cppkafka::Configuration result;
        result.set("bootstrap.servers", Bootstrap);

        result.set("security.protocol", ::ToString(SecurityProtocol));
        if (GetSslCertificate().IsDefined()) {
            result.set("ssl.ca.location", GetSslCertificate().GetPath());
        }
        if (Sasl) {
            result.set("sasl.mechanism", ::ToString(Sasl->GetMechanism()));
            result.set("sasl.username", Sasl->GetUsername());
            result.set("sasl.password", Sasl->GetPassword());
        }
        result.set_error_callback([](cppkafka::KafkaHandleBase&, int error, const std::string& reason) {
            TFLEventLog::Error(TString(reason.data(), reason.size()))("error", error);
            });
        result.set_log_callback([](cppkafka::KafkaHandleBase&, int level, const std::string& facility, const std::string& message) {
            TFLEventLog::Log(TString(message.data(), message.size()), ELogPriority(level))("facility", facility);
            });
        return result;
    }

    THolder<IPQClient> TKafkaConfig::DoConstruct(const IPQConstructionContext& context) const {
        return MakeHolder<TPQKafka>(GetClientId(), *this, context);
    }

    void TKafkaConfig::DoInit(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        const auto children = section->GetAllChildren();

        i32 partition = 0;
        if (dir.GetValue("Partition", partition)) {
            Partition = partition;
        }

        Topic = dir.Value("Topic", Topic);

        SecurityProtocol = dir.Value("SecurityProtocol", SecurityProtocol);
        SslCertificate = dir.Value("SslCertificate", SslCertificate);
        AssertCorrectConfig(dir.GetNonEmptyValue("Bootstrap", Bootstrap), "Bootstrap must be set");
        AssertCorrectConfig(!SslCertificate.IsDefined() || SslCertificate.Exists(), "Certificate file not exists: %s", SslCertificate.GetPath().c_str());
        if(auto sasl = MapFindPtr(children, "Sasl")) {
            Sasl.ConstructInPlace();
            Sasl->Init(*sasl);
        } else {
            AssertCorrectConfig(!EqualToOneOf(SecurityProtocol, ESecurityProtocol::SASL_SSL, ESecurityProtocol::SASL_PLAINTEXT),
                "Section Sasl must be set for " + ::ToString(SecurityProtocol) + " security protocol");
        }
        if (const auto* s = MapFindPtr(children, "Read")) {
            ReadConfig.ConstructInPlace(*this);
            ReadConfig->Init(*s);
        }
        if (const auto* s = MapFindPtr(children, "Write")) {
            WriteConfig.ConstructInPlace(*this);
            WriteConfig->Init(*s);
        }
    }

    void TKafkaConfig::DoToString(IOutputStream& os) const {
        if (!!Partition) {
            os << "Partition: " << *Partition << Endl;
        }
        os << "Topic: " << Topic << Endl;
        os << "Bootstrap: " << Bootstrap << Endl;
        os << "SecurityProtocol: " << SecurityProtocol << Endl;
        if (SslCertificate) {
            os << "SslCertificate: " << SslCertificate << Endl;
        }
        if (Sasl) {
            os << "<Sasl>" << Endl;
            Sasl->ToString(os);
            os << "</Sasl>" << Endl;
        }
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
    }

    void TKafkaConfig::TSasl::Init(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        dir.GetValue("Mechanism", Mechanism);
        dir.GetValue("Username", Username);
        dir.GetValue("Password", Password);

    }

    void TKafkaConfig::TSasl::ToString(IOutputStream& os) const {
        os << "Mechanism: " << Mechanism << Endl;
        os << "Username: " << Username << Endl;
        os << "Password: " << (Password ? MD5::Calc(Password) : TString()) << Endl;
    }

    TKafkaConfig::TReadConfig::TReadConfig(const TKafkaConfig& owner)
        : BaseConfig(owner.BuildKafkaConfig())
    {}

    cppkafka::Configuration TKafkaConfig::TReadConfig::BuildKafkaConfig() const {
        auto result = BaseConfig;
        result.set("group.id", GroupId);
        result.set("enable.auto.commit", false);
        result.set("auto.offset.reset", "earliest");
        return result;
    }

    void TKafkaConfig::TReadConfig::Init(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        AssertCorrectConfig(dir.GetNonEmptyValue("GroupId", GroupId), "GroupId must be initialized");
    }

    void TKafkaConfig::TReadConfig::ToString(IOutputStream& os) const {
        os << "GroupId: " << GroupId << Endl;
    }

    TKafkaConfig::TWriteConfig::TWriteConfig(const TKafkaConfig& owner)
        : BaseConfig(owner.BuildKafkaConfig())
    {}

    cppkafka::Configuration TKafkaConfig::TWriteConfig::BuildKafkaConfig() const {
        auto result = BaseConfig;
        result.set("compression.type", Compression);
        return result;
    }

    void TKafkaConfig::TWriteConfig::Init(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        dir.GetValue("Compression", Compression);
    }

    void TKafkaConfig::TWriteConfig::ToString(IOutputStream& os) const {
        os << "Compression: " << Compression << Endl;
    }

}
