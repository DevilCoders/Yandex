#include "config.h"
#include "local_storage.h"

namespace NRTProc {
    bool TLocalStorageOptions::DeserializeFromJson(const NJson::TJsonValue& value) {
        if (!value.Has("root"))
            return false;
        Root = value["root"].GetString();
        return true;
    }

    NJson::TJsonValue TLocalStorageOptions::SerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("root", Root.GetPath());
        return result;
    }

    bool TLocalStorageOptions::Init(const TYandexConfig::Section* section) {
        if (!section)
            return false;
        const TYandexConfig::Directives& directives = section->GetDirectives();
        directives.GetValue("Root", Root);
        if (Root.IsDefined())
            Root.Fix();
        return true;
    }

    void TLocalStorageOptions::ToString(IOutputStream& os) const {
        os << "Root: " << Root << Endl;
    }

    TAtomicSharedPtr<NRTProc::IVersionedStorage> TLocalStorageOptions::Construct(const TStorageOptions& options) const {
        return MakeAtomicShared<TFSStorage>(options, *this);
    }

    TLocalStorageOptions::TFactory::TRegistrator<TLocalStorageOptions> TLocalStorageOptions::Registrator("LOCAL");
}
