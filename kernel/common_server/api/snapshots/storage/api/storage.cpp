#include "storage.h"

namespace NCS {
    namespace NSnapshots {
        TAPIObjectsManager::TFactory::TRegistrator<TAPIObjectsManager> TAPIObjectsManager::Registrator(TAPIObjectsManager::GetTypeName());
    }
}
