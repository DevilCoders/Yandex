#pragma once

#include <memory>

namespace NCloud::NFileStore::NDaemon
{

////////////////////////////////////////////////////////////////////////////////

struct TBootstrapOptions;
using TBootstrapOptionsPtr = std::shared_ptr<TBootstrapOptions>;

} // namespace NCloud::NFileStore::NDaemon