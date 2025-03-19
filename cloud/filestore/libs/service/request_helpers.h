#pragma once

#include "public.h"

#include <cloud/filestore/public/api/protos/data.pb.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

inline ui64 CalculateRequestSize(const NProto::TWriteDataRequest& request)
{
    return request.GetBuffer().size();
}

inline ui64 CalculateRequestSize(const NProto::TReadDataRequest& request)
{
    return request.GetLength();
}

template <typename T>
ui64 CalculateRequestSize(const T&)
{
    return 0;
}

}   // namespace NCloud::NFileStore
