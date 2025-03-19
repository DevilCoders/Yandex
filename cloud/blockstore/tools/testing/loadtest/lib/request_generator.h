#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/block_range.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/service/request.h>

#include <cloud/blockstore/tools/testing/loadtest/protos/loadtest.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore::NLoadTest {

////////////////////////////////////////////////////////////////////////////////

struct TRequest
{
    EBlockStoreRequest RequestType;
    TBlockRange64 BlockRange;
};

////////////////////////////////////////////////////////////////////////////////

struct IRequestGenerator
{
    virtual ~IRequestGenerator() = default;

    virtual bool Next(TRequest* request) = 0;
    virtual void Complete(TBlockRange64 blockRange) = 0;
    virtual TString Describe() const = 0;
    virtual size_t Size() const = 0;

    virtual TInstant Peek()
    {
        return TInstant::Max();
    }
};

////////////////////////////////////////////////////////////////////////////////

IRequestGeneratorPtr CreateArtificialRequestGenerator(
    ILoggingServicePtr loggingService,
    NProto::TRangeTest range);

IRequestGeneratorPtr CreateRealRequestGenerator(
    ILoggingServicePtr loggingService,
    TString profileLogPath,
    TString diskId,
    bool fullSpeed);

}   // namespace NCloud::NBlockStore::NLoadTest
