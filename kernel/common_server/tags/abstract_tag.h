#pragma once
#include <kernel/common_server/library/storage/records/record.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/abstract/frontend.h>
#include "abstract.h"

class ITag {
private:
    CSA_DEFAULT(ITag, TString, Name);
    CSA_DEFAULT(ITag, TString, Comments);

protected:
    virtual bool DoOnSmartUpsert(const NCS::TDBTagAddress& /*dbTagAddress*/, const IBaseServer& /*server*/, const TDBTagDescription& /*tagDescription*/, NCS::TEntitySession& /*session*/) {
        return true;
    }

public:
    ITag() = default;

    explicit ITag(const TString& name)
        : Name(name)
    {
    }

    using TPtr = TAtomicSharedPtr<ITag>;
    using TFactory = NObjectFactory::TObjectFactory<ITag, TString>;
    virtual ~ITag() = default;

    class TDecoder: public TBaseDecoder {
        RTLINE_ACCEPTOR(TDecoder, Name, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Comments, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, TagObject, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, TagObjectProtoNew, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, TagObjectProtoPacked, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase) {
            Name = GetFieldDecodeIndex("tag_name", decoderBase);
            Comments = GetFieldDecodeIndex("comments", decoderBase);
            TagObject = GetFieldDecodeIndex("tag_object", decoderBase);
            TagObjectProtoNew = TBaseDecoder::GetFieldDecodeIndex("tag_object_proto", decoderBase);
            TagObjectProtoPacked = TBaseDecoder::GetFieldDecodeIndex("tag_object_proto_packed", decoderBase);
        }
    };

    Y_WARN_UNUSED_RESULT virtual bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
    virtual NCS::NStorage::TTableRecord SerializeToTableRecord() const;
    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const;

    virtual NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);

    virtual TString GetClassName() const = 0;
    virtual bool OnBeforeRemove(const NCS::TDBTagAddress& /*dbTagAddress*/, const IBaseServer& /*server*/, const TString& /*userId*/, NCS::TEntitySession& /*session*/) const {
        return true;
    }
    virtual bool OnAfterAdd(const NCS::TDBTagAddress& /*dbTagAddress*/, const IBaseServer& /*server*/, const TString& /*userId*/, NCS::TEntitySession& /*session*/) const {
        return true;
    }

    virtual bool OnSmartUpsert(const NCS::TDBTagAddress& dbTagAddress, const IBaseServer& server, const TDBTagDescription& tagDescription, NCS::TEntitySession& session) final {
        return DoOnSmartUpsert(dbTagAddress, server, tagDescription, session);
    }
};
