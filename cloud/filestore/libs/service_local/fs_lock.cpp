#include "fs.h"

#include "lowlevel.h"

#include <util/string/builder.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

NProto::TAcquireLockResponse TLocalFileSystem::AcquireLock(
    const NProto::TAcquireLockRequest& request)
{
    STORAGE_TRACE("AcquireLock " << DumpMessage(request));

    auto session = GetSession(request);
    auto handle = session->LookupHandle(request.GetHandle());
    if (!handle.IsOpen()) {
        return TErrorResponse(ErrorInvalidHandle(request.GetHandle()));
    }

    bool shared = request.GetLockType() == NProto::E_SHARED;
    if (!NLowLevel::AcquireLock(handle, request.GetOffset(), request.GetLength(), shared)) {
        return TErrorResponse(E_FS_WOULDBLOCK, TStringBuilder()
            << "lock denied for " << handle.GetName().Quote()
            << " at (" << request.GetOffset()
            << ", " << request.GetLength() << ")");
    }

    return {};
}

NProto::TReleaseLockResponse TLocalFileSystem::ReleaseLock(
    const NProto::TReleaseLockRequest& request)
{
    STORAGE_TRACE("ReleaseLock " << DumpMessage(request));

    auto session = GetSession(request);
    auto handle = session->LookupHandle(request.GetHandle());
    if (!handle.IsOpen()) {
        return TErrorResponse(ErrorInvalidHandle(request.GetHandle()));
    }

    NLowLevel::ReleaseLock(handle, request.GetOffset(), request.GetLength());

    return {};
}

NProto::TTestLockResponse TLocalFileSystem::TestLock(
    const NProto::TTestLockRequest& request)
{
    STORAGE_TRACE("TestLock " << DumpMessage(request));

    auto session = GetSession(request);
    auto handle = session->LookupHandle(request.GetHandle());
    if (!handle.IsOpen()) {
        return TErrorResponse(ErrorInvalidHandle(request.GetHandle()));
    }

    bool shared = request.GetLockType() == NProto::E_SHARED;
    if (!NLowLevel::TestLock(handle, request.GetOffset(), request.GetLength(), shared)) {
        return TErrorResponse(E_UNAUTHORIZED, TStringBuilder()
            << "lock denied for " << handle.GetName().Quote()
            << " at (" << request.GetOffset()
            << ", " << request.GetLength() << ")");
    }

    return {};
}

}   // namespace NCloud::NFileStore
