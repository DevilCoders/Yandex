#pragma once

#include "database.h"

#include <kernel/server/server.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NServer {
class THttpServerConfig;
} // namespace NServer

namespace NCbbFast {

class TEnv;

class TCbbServer : public NServer::TServer {
public:
    explicit TCbbServer(const NServer::THttpServerConfig& config, TEnv* env);
    TClientRequest* CreateClient() override;

private:
    TEnv* Env;
};

} // namespace NCbbFast
