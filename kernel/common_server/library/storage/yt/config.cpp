#include "config.h"
#include "storage.h"

namespace NRTProc {

    const TString TYTOptions::ClassName = "YT";

    TAtomicSharedPtr<NRTProc::IVersionedStorage> TYTOptions::Construct(const TStorageOptions& options) const {
        return new TYTStorage(options, *this);
    }

    NJson::TJsonValue TYTOptions::SerializeToJson() const {
        NJson::TJsonValue result;
        result["Server"] = ServerName;
        result["RootPath"] = RootPath;
        return result;
    }

    bool TYTOptions::DeserializeFromJson(const NJson::TJsonValue& value) {
        if (!value.IsMap() || !value.Has("ServerName") || !value.Has("RootPath")) {
            return false;
        }
        ServerName = value["ServerName"].GetStringRobust();
        RootPath = value["RootPath"].GetStringRobust();
        return true;
    }

    TYTOptions::TFactory::TRegistrator<TYTOptions> TYTOptions::Registrator("YT");
}
