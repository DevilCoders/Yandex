#include "config.h"
#include "manager.h"

namespace NCS {
    namespace NSecret {
        TKVManagerConfig::TFactory::TRegistrator<TKVManagerConfig> TKVManagerConfig::Registrator(TKVManagerConfig::GetTypeName());

        TAtomicSharedPtr<NCS::NSecret::ISecretsManager> TKVManagerConfig::Construct(const IBaseServer& /*server*/) const {
            return MakeAtomicShared<TKVManager>( KVConfig->BuildStorage(), IsAllowDuplicate());
        }

        void TKVManagerConfig::DoInit(const TYandexConfig::Section* section) {
            AllowDuplicateFlag = section->GetDirectives().Value("AllowDuplicate", AllowDuplicateFlag);

            auto children = section->GetAllChildren();
            auto it = children.find("Storage");
            if (it != children.end()) {
                KVConfig.Init(it->second);
            }
        }
        void TKVManagerConfig::DoToString(IOutputStream& os) const {
            os << "AllowDuplicate: " << AllowDuplicateFlag << Endl;
            KVConfig.ToString(os);
        }
    }
}
