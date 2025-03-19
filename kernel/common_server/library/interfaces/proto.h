#pragma once
#include <kernel/common_server/util/json_processing.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/config.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/logger/global/global.h>
#include <util/memory/blob.h>
#include <util/system/type_name.h>

class TStubClass {

};

template <class TProtoExt, class TBaseClass = TStubClass,
    NProtobufJson::TJson2ProtoConfig::FldNameMode namedModeJP = NProtobufJson::TJson2ProtoConfig::FldNameMode::FieldNameSnakeCase,
    NProtobufJson::TProto2JsonConfig::FldNameMode namedModePJ = NProtobufJson::TProto2JsonConfig::FldNameMode::FieldNameSnakeCase>
    class INativeProtoSerialization: public TBaseClass {
    private:
        using TBase = TBaseClass;
    public:
        using TBase::TBase;

        virtual ~INativeProtoSerialization() = default;

        using TProto = TProtoExt;
        static constexpr NProtobufJson::TJson2ProtoConfig::FldNameMode NamedModeJP = namedModeJP;
        static constexpr NProtobufJson::TProto2JsonConfig::FldNameMode NamedModePJ = namedModePJ;
        virtual TBlob SerializeToBlob() const final {
            TProto proto;
            SerializeToProto(proto);
            return TBlob::FromString(proto.SerializeAsString());
        }

        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromBlob(const TBlob& data) final {
            TProto proto;
            if (!proto.ParseFromArray(data.Data(), data.Size())) {
                ERROR_LOG << "Cannot parse data from proto in regular process" << Endl;
                return false;
            }
            return DeserializeFromProto(proto);
        }
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonValue) {
            TProto proto;
            try {
                NProtobufJson::TJson2ProtoConfig config;
                config.SetFieldNameMode(namedModeJP);
                NProtobufJson::Json2Proto(jsonValue, proto, config);
            } catch (...) {
                TFLEventLog::Log("cannot_convert_json2proto")("class", TypeName<TProtoExt>())("error", CurrentExceptionMessage());
                return false;
            }
            if (!DeserializeFromProto(proto)) {
                TFLEventLog::Log("cannot_deserialize_proto")("class", TypeName<TProtoExt>());
                return false;
            }
            return true;
        }
        virtual NJson::TJsonValue SerializeToJson() const {
            TProto proto;
            SerializeToProto(proto);
            NJson::TJsonValue result;
            NProtobufJson::TProto2JsonConfig config;
            config.SetFieldNameMode(namedModePJ);
            try {
                NProtobufJson::Proto2Json(proto, result, config);
                return result;
            } catch (...) {
                return NJson::JSON_NULL;
            }

        }
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromProto(const TProto& proto) = 0;
        virtual void SerializeToProto(TProto& proto) const = 0;

        virtual TProtoExt ToProto() const final {
            TProtoExt result;
            SerializeToProto(result);
            return result;
        }
};

