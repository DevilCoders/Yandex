#pragma once

#include <util/generic/vector.h>

class IOutputStream;

namespace NCloud::NFileStore {

namespace NProto {

////////////////////////////////////////////////////////////////////////////////

class TProfileLogRecord;

}   // namespace NProto

////////////////////////////////////////////////////////////////////////////////

TVector<ui32> GetItemOrder(const NProto::TProfileLogRecord& record);

void DumpRequest(
    const NProto::TProfileLogRecord& record,
    int i,
    IOutputStream* out);

TString RequestName(const ui32 requestType);

}   // namespace NCloud::NFileStore
