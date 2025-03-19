#include "config.h"
#include "manager.h"

namespace NCS {
    namespace NSecret {
        TFakeManagerConfig::TFactory::TRegistrator<TFakeManagerConfig> TFakeManagerConfig::Registrator(TFakeManagerConfig::GetTypeName());

        TAtomicSharedPtr<NCS::NSecret::ISecretsManager> TFakeManagerConfig::Construct(const IBaseServer& /*server*/) const {
            return MakeAtomicShared<TFakeManager>();
        }

    }
}
