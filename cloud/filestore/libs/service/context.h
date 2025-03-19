#pragma once

#include "public.h"
#include "request.h"

#include <cloud/storage/core/libs/common/error.h>

#include <library/cpp/lwtrace/shuttle.h>

#include <util/generic/ptr.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

struct TCallContext
    : TAtomicRefCount<TCallContext>
{
private:
    TAtomic RequestStartedCycles = 0;

public:
    const ui64 RequestId = 0;
    ui64 RequestSize = 0;
    EFileStoreRequest RequestType = EFileStoreRequest::MAX;
    NProto::TError Error;

    NLWTrace::TOrbit LWOrbit;

    TCallContext(ui64 requestId = 0)
        : RequestId(requestId)
    {}

    virtual ~TCallContext() = default;

    ui64 GetStartedCycles() {
        return AtomicGet(RequestStartedCycles);
    }

    void SetStartedCycles(ui64 cycles) {
        AtomicSet(RequestStartedCycles, cycles);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TServerCallContext: public TCallContext
{
    const TString FileSystemId;
    TInstant RequestStartedTs = TInstant::Now();

    TServerCallContext(ui64 reqId, TString fileSystemId)
        : TCallContext(reqId)
        , FileSystemId(std::move(fileSystemId))
    {}
};

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_TRACK(probe, context, type, ...)                             \
    LWTRACK(probe, context->LWOrbit, type, context->RequestId,                 \
        ##__VA_ARGS__);                                                        \
// FILESTORE_TRACK

}   // namespace NCloud::NFileStore
