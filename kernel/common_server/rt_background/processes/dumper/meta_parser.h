#pragma once

#include <library/cpp/object_factory/object_factory.h>

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/config.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/string.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/storage/records/t_record.h>

class IDumperMetaParser {
public:
    using TPtr = TAtomicSharedPtr<IDumperMetaParser>;
    using TFactory = NObjectFactory::TObjectFactory<IDumperMetaParser, TString>;

    virtual ~IDumperMetaParser() {
    }
    virtual NJson::TJsonValue ParseMeta(const NCS::NStorage::TDBValue& dataExt) const = 0;

    virtual bool NeedFullRecord() const;
    virtual NJson::TJsonValue ParseMeta(const NCS::NStorage::TTableRecordWT& tr) const;
};

template <class TProto>
class TProtoBytesMetaParser: public IDumperMetaParser {
public:
    virtual NJson::TJsonValue ParseMeta(const NCS::NStorage::TDBValue& dataExt) const override {
        const TString data = NCS::NStorage::TDBValueOperator::SerializeToString(dataExt);
        TProto proto;
        if (!proto.ParseFromArray(data.c_str(), data.size())) {
            return NJson::JSON_MAP;
        }
        NJson::TJsonValue result;
        NProtobufJson::TProto2JsonConfig config;
        config.SetFieldNameMode(NProtobufJson::TProto2JsonConfig::FldNameMode::FieldNameSnakeCase);
        try {
            NProtobufJson::Proto2Json(proto, result, config);
            return result;
        } catch (...) {
            return NJson::JSON_MAP;
        }
        return result;
    }
};

template <class TProto>
class TProtoMetaParser: public IDumperMetaParser {
public:
    virtual NJson::TJsonValue ParseMeta(const NCS::NStorage::TDBValue& dataExt) const override {
        const TString data = NCS::NStorage::TDBValueOperator::SerializeToString(dataExt);
        TProto proto;
        const TString protoStr = Base64Decode(data);
        if (!proto.ParseFromArray(protoStr.c_str(), protoStr.size())) {
            return NJson::JSON_MAP;
        }
        NJson::TJsonValue result;
        NProtobufJson::TProto2JsonConfig config;
        config.SetFieldNameMode(NProtobufJson::TProto2JsonConfig::FldNameMode::FieldNameSnakeCase);
        try {
            NProtobufJson::Proto2Json(proto, result, config);
            return result;
        } catch (...) {
            return NJson::JSON_MAP;
        }
        return result;
    }
};
