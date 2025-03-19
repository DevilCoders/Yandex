#include "serialize.h"

#include <kernel/prs_log/serializer/factor_storage/serializer.h>
#include <kernel/prs_log/serializer/uniform_bound/serializer.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/streams/lz/lz.h>
#include <util/generic/ptr.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/stream/mem.h>
#include <util/string/cast.h>

namespace {
    TString Compress(const TStringBuf& data, NPrsLog::ECompressionMethod compressionMethod) {
        TString result;
        TStringOutput out(result);

        switch (compressionMethod) {
            case NPrsLog::ECompressionMethod::CM_LZ4: {
                TLz4Compress comp(&out);
                comp.Write(data);
                comp.Flush();
                break;
            }

            case NPrsLog::ECompressionMethod::CM_NONE: {
                result = TString(data);
                break;
            }
        }

        return result;
    }

    TString Decompress(const TStringBuf& data, NPrsLog::ECompressionMethod compressionMethod) {
        TMemoryInput in(data.data(), data.size());
        TString result;

        switch (compressionMethod) {
            case NPrsLog::ECompressionMethod::CM_LZ4: {
                TLz4Decompress decomp(&in);
                result = decomp.ReadAll();
                break;
            }

            case NPrsLog::ECompressionMethod::CM_NONE: {
                result = TString(data);
                break;
            }
        }

        return result;
    }
}

namespace NPrsLog {
    THolder<IWebPrsSerializer> GetSerializer(const ESerializerType serializerType) {
        switch (serializerType) {
            case ESerializerType::UniformBound:
                return MakeHolder<TUniformBoundSerializer>();
            case ESerializerType::FactorStorage:
                return MakeHolder<TFactorStorageSerializer>();
        }
    }

    TString SerializeToBase64(const TWebData& web, const ESerializerType serializerType) {
        THolder<IWebPrsSerializer> serializer = GetSerializer(serializerType);

        TString sType(reinterpret_cast<const char*>(&serializerType), sizeof(serializerType));
        TString result = serializer->Serialize(web);
        return Base64Encode(sType + result);
    }

    TWebData DeserializeFromBase64(const TString& base64) {
        TString decoded = Base64Decode(base64);
        const ui16 sType = *reinterpret_cast<const ui16*>(decoded.data());

        THolder<IWebPrsSerializer> serializer = GetSerializer(static_cast<ESerializerType>(sType));
        return serializer->Deserialize(TString(decoded.data() + sizeof(ui16), decoded.size() - sizeof(ui16)));
    }

    TString SerializeToProtoBase64(const NPrsLogProto::TLog& log, ECompressionMethod compressionMethod) {
        TString data;
        Y_PROTOBUF_SUPPRESS_NODISCARD log.SerializeToString(&data);

        if (compressionMethod != ECompressionMethod::CM_NONE) {
            NPrsLogProto::TLog compressedLog;
            compressedLog.SetCompressionMethod(ToString(compressionMethod));
            compressedLog.SetCompressedLog(Compress(data, compressionMethod));
            data.clear();
            Y_PROTOBUF_SUPPRESS_NODISCARD compressedLog.SerializeToString(&data);
        }

        return Base64Encode(data);
    }

    NPrsLogProto::TLog DeserializeFromProtoBase64(const TString& data) {
        NPrsLogProto::TLog log;
        Y_PROTOBUF_SUPPRESS_NODISCARD log.ParseFromString(Base64Decode(data));

        ECompressionMethod compressionMethod = FromString<ECompressionMethod>(log.GetCompressionMethod());

        if (compressionMethod != ECompressionMethod::CM_NONE) {
            NPrsLogProto::TLog decompressedLog;
            Y_PROTOBUF_SUPPRESS_NODISCARD decompressedLog.ParseFromString(Decompress(log.GetCompressedLog(), compressionMethod));
            return decompressedLog;
        }

        return log;
    }
} // NPrsLog
