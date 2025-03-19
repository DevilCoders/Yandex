#pragma once

#include <util/generic/strbuf.h>

#include <memory>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TDiskAgentConfig;
using TDiskAgentConfigPtr = std::shared_ptr<TDiskAgentConfig>;

constexpr TStringBuf BackgroundOpsSessionId = "migration";

}   // namespace NCloud::NBlockStore::NStorage
