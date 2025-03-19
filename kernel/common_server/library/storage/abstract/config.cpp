#include "config.h"
#include <library/cpp/mediator/global_notifications/system_status.h>
#include "database.h"

namespace NCS {
    namespace NStorage {
        void TDatabaseConfig::Init(const TYandexConfig::Section* section) {
            const auto& dirs = section->GetDirectives();
            AssertCorrectConfig(dirs.GetNonEmptyValue("Type", Type), "No Type for TDatabaseConfig");
            ConfigImpl = IDatabaseConfig::TFactory::Construct(Type);
            AssertCorrectConfig(!!ConfigImpl, "Unknown Type %s for TDatabaseConfig", Type.c_str());
            ConfigImpl->Init(section);
        }

        void TDatabaseConfig::ToString(IOutputStream& os) const {
            os << "Type: " << Type << Endl;
            ConfigImpl->ToString(os);
        }

        IDatabase::TPtr TDatabaseConfig::ConstructDatabase(const IDatabaseConstructionContext* context /*= nullptr*/) const {
            return ConfigImpl->ConstructDatabase(context);
        }

        const NCS::ITvmManager* IDatabaseConstructionContext::GetTvmManager() const {
            return nullptr;
        }

    }
}
