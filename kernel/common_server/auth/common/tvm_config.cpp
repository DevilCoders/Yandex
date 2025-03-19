#include "tvm_config.h"
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <util/string/join.h>
#include <library/cpp/logger/global/global.h>
#include <util/stream/file.h>
#include <library/cpp/digest/md5/md5.h>

NTvmAuth::NTvmApi::TClientSettings::TDstMap TTvmConfig::BuildDestinationsMap() const {
    NTvmAuth::NTvmApi::TClientSettings::TDstMap destinations;
    for (const auto& [id, aliases] : DestinationClientIds) {
        for (const auto& alias : aliases) {
            AssertCorrectConfig(destinations.emplace(alias, id).second, "duplicated tvm destination aliales");
        }
    };
    return destinations;
}

void TTvmConfig::Init(const TYandexConfig::Section* section) {
    CHECK_WITH_LOG(section);
    const auto& directives = section->GetDirectives();
    BlackboxEnv = directives.Value("BlackboxEnv", NTvmAuth::EBlackboxEnv::Test);
    Cache = directives.Value("Cache", Cache);
    Secret = directives.Value("Secret", Secret);
    SecretFile = directives.Value("SecretFile", SecretFile);
    SelfClientId = directives.Value("SelfClientId", SelfClientId);
    Name = directives.Value("Name", ::ToString(SelfClientId));
    DefaultClientFlag = directives.Value("IsDefaultClient", DefaultClientFlag);
    Host = directives.Value("Host", Host);
    Port = directives.Value("Port", Port);
    StringSplitter(
        directives.Value("DestinationClientIds", TString())
    ).SplitBySet(" ,").SkipEmpty().Consume([this](const TStringBuf pair) {
        TStringBuf idstr, aliases;
        pair.Split("=", idstr, aliases);
        auto& aliasesVector = DestinationClientIds[FromString<ui32>(idstr)];
        if (aliases) {
            StringSplitter(aliases).Split('|').SkipEmpty().AddTo(&aliasesVector);
        } else {
            aliasesVector.emplace_back(idstr);
        }
    });

    if (SecretFile && !Secret) {
        Secret = TIFStream(SecretFile).ReadAll();
    }
}

void TTvmConfig::ToString(IOutputStream& os) const {
    os << "Cache: " << Cache << Endl;
    os << "BlackboxEnv: " << BlackboxEnv << Endl;
    os << "SelfClientId: " << SelfClientId << Endl;
    os << "Name: " << Name << Endl;
    TVector<TString> dsts;
    for (const auto& [id, aliases]: DestinationClientIds) {
        dsts.emplace_back(::ToString(id) + "=" + JoinSeq('|', aliases));
    }
    os << "DestinationClientIds: " << JoinSeq(",", dsts) << Endl;
    if (SecretFile) {
        os << "SecretFile: " << SecretFile << Endl;
    } else {
        os << "Secret: " << MD5::Calc(Secret) << Endl;
    }
    os << "IsDefaultClient: " << DefaultClientFlag << Endl;
    os << "Host: " << Host << Endl;
    os << "Port: " << Port << Endl;
}

