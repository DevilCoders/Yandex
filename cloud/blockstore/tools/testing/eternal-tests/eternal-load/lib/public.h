#pragma once

#include <util/system/defaults.h>

#include <memory>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

struct TBlockData
{
    ui64 RequestNumber;
    ui64 PartNumber;
    ui64 BlockIndex;
    ui64 RangeIdx;
    ui64 RequestTimestamp;
    ui64 TestTimestamp;
    ui64 TestId;
    ui64 Checksum;
};

////////////////////////////////////////////////////////////////////////////////

struct IRequestGenerator;
using IRequestGeneratorPtr = std::shared_ptr<IRequestGenerator>;

struct ITestExecutor;
using ITestExecutorPtr = std::shared_ptr<ITestExecutor>;

struct IConfigHolder;
using IConfigHolderPtr = std::shared_ptr<IConfigHolder>;

}   // namespace NCloud::NBlockStore
