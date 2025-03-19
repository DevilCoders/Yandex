#include "case.h"
#include <library/cpp/json/json_reader.h>
#include <library/cpp/config/config.h>
#include <library/cpp/config/extra/yconf.h>

namespace NCS {

    void IEmulationCase::Init(const TYandexConfig::Section* section) {
        TStringStream ss;
        TYandexConfig::PrintSectionConfig(section, ss, false);
        TStringStream ssJson;
        NConfig::TConfig::FromMarkup(ss).ToJson(ssJson);
        NJson::TJsonValue jsonInfo;
        AssertCorrectConfig(NJson::ReadJsonFastTree(ssJson.Str(), &jsonInfo), "incorrect config - cannot serialize as json");
        INFO_LOG << jsonInfo["Case"] << Endl;
        AssertCorrectConfig(DeserializeFromJson(jsonInfo["Case"]), "incorrect config - cannot parse json");
    }

    void IEmulationCase::ToString(IOutputStream& os) const {
        NJson::TJsonValue jsonInfo = SerializeToJson();
        const TString strJson = jsonInfo.GetStringRobust();
        NConfig::Json2RawYConf(os, strJson);
    }

}
