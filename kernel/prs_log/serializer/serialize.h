#pragma once

#include <kernel/prs_log/data_types/web.h>
#include <kernel/prs_log/data_types/log.pb.h>

namespace NPrsLog {
    enum class ESerializerType : ui16 {
        UniformBound = 1,
        FactorStorage = 2,
    };

    enum class ECompressionMethod {
        CM_NONE = 0     /* "none" */,
        CM_LZ4 = 1      /* "lz4" */,
    };

    TString SerializeToBase64(const TWebData& web, const ESerializerType serializerType = ESerializerType::UniformBound);
    TWebData DeserializeFromBase64(const TString& base64);

    TString SerializeToProtoBase64(const NPrsLogProto::TLog& log, ECompressionMethod compressionMethod = ECompressionMethod::CM_NONE);
    NPrsLogProto::TLog DeserializeFromProtoBase64(const TString& data);
} // NPrsLog
