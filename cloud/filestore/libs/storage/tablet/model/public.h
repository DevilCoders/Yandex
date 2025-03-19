#pragma once

#include <util/system/defaults.h>

#include <memory>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

using TCompactionCounter = std::pair<ui32, ui32>;

struct IBlockBuffer;
using IBlockBufferPtr = std::shared_ptr<IBlockBuffer>;

struct IBlockIterator;
using IBlockIteratorPtr = std::shared_ptr<IBlockIterator>;

struct IBlockLocation2RangeIndex;
using IBlockLocation2RangeIndexPtr = std::shared_ptr<IBlockLocation2RangeIndex>;

}   // namespace NCloud::NFileStore::NStorage
