#include "config.h"
#include "queue.h"
#include <library/cpp/logger/init_context/yconf.h>
#include <library/cpp/mediator/global_notifications/system_status.h>

namespace NCS {
    namespace NPQ {
        namespace NDatabase {
            TConfig::TFactory::TRegistrator<TConfig> TConfig::Registrator(TConfig::GetTypeName());
            THolder<IPQClient> TConfig::DoConstruct(const IPQConstructionContext& context) const {
                return MakeHolder<TQueue>(GetClientId(), context.GetDatabase(DBName), context.GetReadSettings());
            }

            void TConfig::DoInit(const TYandexConfig::Section* section) {
                DBName = section->GetDirectives().Value("DBName", DBName);
                AssertCorrectConfig(!!DBName, "empty db name for background tasks manager");
            }

            void TConfig::DoToString(IOutputStream& os) const {
                os << "DBName: " << DBName << Endl;
            }
        }
    }
}
