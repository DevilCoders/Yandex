#include "item.h"
#include <library/cpp/config/config.h>
#include <library/cpp/config/extra/yconf.h>

void IItemPermissions::Init(const TYandexConfig::Section* section) {
    TStringStream ss;
    TYandexConfig::PrintSectionConfig(section, ss, false);
    TStringStream ssJson;
    NConfig::TConfig::FromMarkup(ss).ToJson(ssJson);

    NJson::TJsonValue jsonInfo;
    AssertCorrectConfig(NJson::ReadJsonFastTree(ssJson.Str(), &jsonInfo), "incorrect permissions config - cannot serialize as json");
    AssertCorrectConfig(DeserializeFromJson(jsonInfo[section->Name]), "incorrect permissions config - cannot parse json");
}

void IItemPermissions::ToString(IOutputStream& os) const {
    NJson::TJsonValue jsonInfo = SerializeToJson();
    const TString strJson = jsonInfo.GetStringRobust();
    NConfig::Json2RawYConf(os, strJson);
}
