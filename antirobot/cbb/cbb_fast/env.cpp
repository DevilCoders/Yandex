#include "env.h"

#include "database.h"

#include <maps/libs/pgpool/include/pgpool3.h>

namespace NCbbFast {

TEnv::TEnv(const TConfig& config)
    : Tvm(config)
    , Database(config) {
}

} // namespace NCbbFast
