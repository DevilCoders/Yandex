#include "config.h"
#include "zoo_storage.h"

namespace NRTProc {

    bool TZooStorageOptions::DeserializeFromJson(const NJson::TJsonValue& value) {
        if (!value.Has("root"))
            return false;
        Root = value["root"].GetString();

        if (!value.Has("zoo_address"))
            return false;
        Address = value["zoo_address"].GetString();

        if (!value.Has("zoo_log_level"))
            LogLevel = 3; // LL_INFO
        else
            LogLevel = value["zoo_log_level"].GetInteger();

        return true;
    }

    NJson::TJsonValue TZooStorageOptions::SerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("root", Root);
        result.InsertValue("zoo_address", Address);
        result.InsertValue("zoo_log_level", LogLevel);
        return result;
    }

    bool TZooStorageOptions::Init(const TYandexConfig::Section* section) {
        if (!section)
            return false;
        const TYandexConfig::Directives& directives = section->GetDirectives();
        directives.GetValue("Root", Root);
        directives.GetValue("Address", Address);
        directives.GetValue("LogLevel", LogLevel);
        return true;
    }

    void TZooStorageOptions::ToString(IOutputStream& os) const {
        os << "Root: " << Root << Endl;
        os << "Address: " << Address << Endl;
        os << "LogLevel: " << LogLevel << Endl;
    }

    TAtomicSharedPtr<NRTProc::IVersionedStorage> TZooStorageOptions::Construct(const TStorageOptions& options) const {
        AssertCorrectConfig(!!Root, "Incorrect Root value");
        AssertCorrectConfig(!!Address, "Incorrect Address value");
        return MakeAtomicShared<TZooStorage>(options, *this);
    }

    TZooStorageOptions::TFactory::TRegistrator<TZooStorageOptions> TZooStorageOptions::Registrator("ZOO");
}
