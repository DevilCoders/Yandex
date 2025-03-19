#pragma once

#include <cloud/storage/core/libs/common/error.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

//
// Nodes.
//

NProto::TError ErrorInvalidParent(ui64 nodeId);
NProto::TError ErrorInvalidTarget(ui64 nodeId, const TString& name = {});
NProto::TError ErrorAlreadyExists(const TString& path);

//
// Directories.
//

NProto::TError ErrorIsDirectory(ui64 nodeId);
NProto::TError ErrorIsNotDirectory(ui64 nodeId);
NProto::TError ErrorIsNotEmpty(ui64 nodeId);

//
// Limits.
//

NProto::TError ErrorNameTooLong(const TString& name);
NProto::TError ErrorSymlinkTooLong(const TString& link);
NProto::TError ErrorMaxLink(ui64 nodeId);
NProto::TError ErrorFileTooBig();
NProto::TError ErrorNoSpaceLeft();
NProto::TError ErrorAttributeNameTooLong(const TString& name);
NProto::TError ErrorAttributeValueTooBig(const TString& name);

//
// Arguments.
//

NProto::TError ErrorInvalidArgument();
NProto::TError ErrorInvalidHandle();
NProto::TError ErrorInvalidHandle(ui64 handle);

//
// Attriubutes
//

NProto::TError ErrorInvalidAttribute(const TString& name);
NProto::TError ErrorAttributeAlreadyExists(const TString& name);
NProto::TError ErrorAttributeNotExists(const TString& name);

//
// Locks
//

NProto::TError ErrorIncompatibleLocks();

//
// Session.
//

NProto::TError ErrorInvalidSession(const TString& clientId, const TString& sessionId);
NProto::TError ErrorInvalidCheckpoint(const TString& checkpointId);

////////////////////////////////////////////////////////////////////////////////

int FileStoreErrorToErrno(int error);
int ErrnoToFileStoreError(int error);

}   // namespace NCloud::NFileStore
