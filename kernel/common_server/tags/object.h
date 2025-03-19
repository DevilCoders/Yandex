#pragma once

#include "abstract.h"
#include "abstract_tag.h"
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/library/scheme/scheme.h>

namespace NCS {
    namespace NStorage {
        template <class T>
        class TTagIdWriter {
        public:
            static bool Write(NStorage::TTableRecord& /*tr*/, const TString& /*fieldId*/, const TString& /*tagId*/) {
                Y_UNREACHABLE();
                return false;
            }
        };

        template <>
        class TTagIdWriter<TString> {
        public:
            static bool Write(NStorage::TTableRecord& tr, const TString& fieldId, const TString& tagId) {
                auto guid = GetUuid(tagId);
                if (!guid) {
                    return false;
                }
                tr.Set(fieldId, guid);
                return true;
            }
        };

        template <>
        class TTagIdWriter<ui32> {
        public:
            static bool Write(NStorage::TTableRecord& tr, const TString& fieldId, const ui32& tagId) {
                tr.Set(fieldId, tagId);
                return true;
            }
        };

        template <>
        class TTagIdWriter<ui64> {
        public:
            static bool Write(NStorage::TTableRecord& tr, const TString& fieldId, const ui64& tagId) {
                tr.Set(fieldId, tagId);
                return true;
            }
        };
    }
    template <class TObjectIdExt, class TTagIdExt, class TPerformerId = TString>
    class TDBTagImpl: public TInterfaceContainer<ITag> {
    public:
        using TObjectId = TObjectIdExt;
        using TTagId = TTagIdExt;
    private:
        using TBase = TInterfaceContainer<ITag>;
        CS_ACCESS(TDBTagImpl, TObjectId, ObjectId, TObjectId());
        CS_ACCESS(TDBTagImpl, TTagId, TagId, TTagId());
        CS_ACCESS(TDBTagImpl, TPerformerId, PerformerId, TPerformerId());
    public:
        TDBTagImpl() = default;
        TDBTagImpl(const TDBTagImpl& dbTag) = default;
        using TBase::TBase;

        const TString& GetName() const {
            if (!!Object) {
                return Object->GetName();
            } else {
                return Default<TString>();
            }
        }

        TDBTagAddress GetAddress() const {
            return TDBTagAddress(ObjectId, TagId);
        }

        static TString GetObjectIdFieldName() {
            return "object_id";
        }

        static TString GetIdFieldName() {
            return "tag_id";
        }

        using TId = TTagId;

        const TId GetInternalId() const {
            return TagId;
        }

        class TDecoder: public TBase::TDecoder {
            using TBaseDecoder = TInterfaceContainer<ITag>::TDecoder;
            RTLINE_ACCEPTOR(TDecoder, ObjectId, i32, -1);
            RTLINE_ACCEPTOR(TDecoder, TagId, i32, -1);
            RTLINE_ACCEPTOR(TDecoder, PerformerId, i32, -1);
        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderBase)
                : TBaseDecoder(decoderBase) {
                ObjectId = GetFieldDecodeIndex("object_id", decoderBase);
                TagId = GetFieldDecodeIndex("tag_id", decoderBase);
                PerformerId = GetFieldDecodeIndex("performer_id", decoderBase);
            }
        };

        NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const {
            NCS::NScheme::TScheme result;
            if (!!Object) {
                result = Object->GetScheme(server);
            }
            if (std::is_integral<TId>::value) {
                result.Add<TFSNumeric>("tag_id").SetReadOnly(true);
            } else {
                result.Add<TFSString>("tag_id").SetReadOnly(true);
            }
            if (std::is_integral<TObjectId>::value) {
                result.Add<TFSNumeric>("object_id");
            } else {
                result.Add<TFSString>("object_id");
            }
            if (std::is_integral<TPerformerId>::value) {
                result.Add<TFSNumeric>("performer_id");
            } else {
                result.Add<TFSString>("performer_id");
            }
            return result;
        }

        static NCS::NScheme::TScheme GetSchemeFixed(const IBaseServer& server) {
            NCS::NScheme::TScheme result = TBase::GetScheme(server);
            if (std::is_integral<TPerformerId>::value) {
                result.Add<TFSNumeric>("performer_id");
            } else {
                result.Add<TFSString>("performer_id");
            }
            if (std::is_integral<TObjectId>::value) {
                result.Add<TFSNumeric>("object_id");
            } else {
                result.Add<TFSString>("object_id");
            }
            if (std::is_integral<TId>::value) {
                result.Add<TFSNumeric>("tag_id").SetReadOnly(true);
            } else {
                result.Add<TFSString>("tag_id").SetReadOnly(true);
            }
            return result;
        }

        TDBTagImpl GetDeepCopy() const {
            TDBTagImpl newObject;
            CHECK_WITH_LOG(TBaseDecoder::DeserializeFromTableRecordCommon(newObject, SerializeToTableRecord().BuildWT(), false));
            return newObject;
        }

        Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            auto gLogging = TFLRecords::StartContext().Method("TDBTag::DeserializeWithDecoder");
            READ_DECODER_VALUE(decoder, values, ObjectId);
            if (decoder.GetPerformerId() >= 0) {
                READ_DECODER_VALUE_OPT(decoder, values, PerformerId);
            }
            READ_DECODER_VALUE(decoder, values, TagId);
            gLogging("tag_id", TagId);
            if (!TBase::DeserializeWithDecoder(decoder, values)) {
                TFLEventLog::Error("cannot parse interface container");
                return false;
            }
            auto td = ITagDescriptions::Instance().GetTagDescription(Object->GetName());
            if (!td) {
                TFLEventLog::Error("Cannot restore tag description")("tag_name", Object->GetName());
                return false;
            }
            if (td->GetClassName() != Object->GetClassName()) {
                TFLEventLog::Error("Inconsistency tag description with tag")("tag_name", Object->GetName())("td_class_name", td->GetClassName())("tag_class_name", Object->GetClassName());
                return false;
            }
            return true;
        }

        NJson::TJsonValue SerializeToJson() const {
            NJson::TJsonValue result = TBase::SerializeToJson();
            result.InsertValue("tag_id", TagId);
            result.InsertValue("object_id", ObjectId);
            result.InsertValue("performer_id", PerformerId);
            return result;
        }

        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TBase::DeserializeFromJson(jsonInfo)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "tag_id", TagId)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "object_id", ObjectId)) {
                return false;
            }
            return true;
        }

        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo, const TString& className) {
            if (!TBase::DeserializeFromJson(jsonInfo, className)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "tag_id", TagId)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "object_id", ObjectId)) {
                return false;
            }
            return true;
        }

        NStorage::TTableRecord SerializeToTableRecord() const {
            NStorage::TTableRecord result = TBase::SerializeToTableRecord();
            if (!!TagId) {
                NStorage::TTagIdWriter<TTagIdExt>::Write(result, "tag_id", TagId);
            }
            if (!!ObjectId) {
                result.Set("object_id", ObjectId);
            }
            if (!!PerformerId) {
                result.Set("performer_id", PerformerId);
            }
            return result;
        }

    };

}

class TDBTag: public NCS::TDBTagImpl<TString, TString> {
private:
    using TBase = NCS::TDBTagImpl<TString, TString>;
public:
    using TBase::TBase;
};
