#pragma once
#include <kernel/common_server/library/interfaces/proto.h>
#include "abstract_tag.h"

template <class TProto>
class ITagProto: public INativeProtoSerialization<TProto, ITag> {
private:
    using TBase = INativeProtoSerialization<TProto, ITag>;
public:
    using TBase::TBase;
    using TBase::GetClassName;
    using TBase::DeserializeFromProto;
    using TBase::SerializeToProto;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeWithDecoder(const typename TBase::TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) override {
        auto gLogging = TFLRecords::StartContext().Method("ITagProto::DeserializeWithDecoder")("class_name", GetClassName());
        if (!TBase::DeserializeWithDecoder(decoder, values)) {
            TFLEventLog::Error("cannot parse base object");
            return false;
        }
        TProto proto;
        if (TTagStorageCustomization::ReadNewBinaryData) {
            if (decoder.GetTagObjectProtoPacked() != -1 && !!values[decoder.GetTagObjectProtoPacked()]) {
                if (!decoder.GetProtoValueBytesPackedAuto(decoder.GetTagObjectProtoPacked(), values, proto)) {
                    TFLEventLog::Error("cannot unpack bytes packed auth");
                    return false;
                }
            } else if (!decoder.GetProtoValueBytes(decoder.GetTagObjectProtoNew(), values, proto)) {
                TFLEventLog::Error("cannot unpack bytes proto");
                return false;
            }
        } else if (TTagStorageCustomization::ReadOldBinaryData) {
            NCommonServerProto::TTagObject tagProto;
            if (!decoder.template GetProtoValueDeprecated<NCommonServerProto::TTagObject>(decoder.GetTagObject(), values, tagProto)) {
                TFLEventLog::Error("cannot use GetProtoValueDeprecated");
                return false;
            }
            if (!TBaseDecoder::DeserializeProtoFromBase64String(tagProto.GetTagData(), proto)) {
                TFLEventLog::Error("cannot use DeserializeProtoFromBase64String");
                return false;
            }
        } else {
            return false;
        }
        if (!DeserializeFromProto(proto)) {
            TFLEventLog::Error("cannot parse from proto");
            return false;
        }

        return true;
    }

    virtual NStorage::TTableRecord SerializeToTableRecord() const override {
        NStorage::TTableRecord result = TBase::SerializeToTableRecord();
        TProto proto;
        SerializeToProto(proto);
        if (TTagStorageCustomization::WriteNewBinaryData) {
            result.SetProtoBytes("tag_object_proto", proto);
        }
        if (TTagStorageCustomization::WritePackedBinaryData) {
            TString protoStr = proto.SerializeAsString();
            NCS::NStorage::EDataCodec codec = NCS::NStorage::EDataCodec::Null;
            if (protoStr.size() > 1024) {
                codec = NCS::NStorage::EDataCodec::LZ4;
            } else if (protoStr.size() > 128) {
                codec = NCS::NStorage::EDataCodec::ZLib;
            }
            result.SetBytesPackedWithCodec(codec, "tag_object_proto_packed", protoStr);
        }
        if (TTagStorageCustomization::WriteOldBinaryData) {
            NCommonServerProto::TTagObject tagProto;
            tagProto.SetTagData(TBaseDecoder::SerializeProtoToBase64String(proto));
            result.SetProtoDeprecated("tag_object", tagProto);
        }
        return result;
    }

    virtual NJson::TJsonValue SerializeToJson() const override {
        NJson::TJsonValue result = TBase::SerializeToJson();
        result.InsertValue("tag_name", TBase::GetName());
        result.InsertValue("comments", TBase::GetComments());
        return result;
    }
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
        if (!TBase::DeserializeFromJson(jsonInfo)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "tag_name", TBase::MutableName(), true) || !TBase::GetName()) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "comments", TBase::MutableComments())) {
            return false;
        }
        return true;
    }
};

template <class TProto>
class ITagProtoWithNativeScheme: public ITagProto<TProto> {
private:
    using TBase = ITagProto<TProto>;
public:
    using TBase::TBase;
    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme result = TBase::GetScheme(server);
        NProtobufJson::TProto2JsonConfig config;
        config.SetFieldNameMode(TBase::NamedModePJ);
        result.SetProto<TProto>(config);
        return result;
    }
};
