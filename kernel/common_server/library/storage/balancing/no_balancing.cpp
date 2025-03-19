#include "no_balancing.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            TNoBalancingPolicy::TFactory::TRegistrator<TNoBalancingPolicy> TNoBalancingPolicy::Registrator(TNoBalancingPolicy::GetTypeName());
        }
    }
}
