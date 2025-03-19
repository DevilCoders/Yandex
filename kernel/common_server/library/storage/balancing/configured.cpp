#include "configured.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            TConfiguredBalancingPolicy::TFactory::TRegistrator<TConfiguredBalancingPolicy> TConfiguredBalancingPolicy::Registrator(TConfiguredBalancingPolicy::GetTypeName());
        }
    }
}
