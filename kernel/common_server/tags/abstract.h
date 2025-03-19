#pragma once
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/storage/records/record.h>

class TTagStorageCustomization {
public:
    static bool WriteOldBinaryData;
    static bool ReadOldBinaryData;
    static bool WriteNewBinaryData;
    static bool ReadNewBinaryData;
    static bool WritePackedBinaryData;
};

class TDBTagDescription;

namespace NCS {
    class TDBTagAddress {
    private:
        CSA_READONLY_DEF(TString, ObjectId);
        CSA_READONLY_DEF(TString, TagId);
    public:
        template <class TObjectId, class TTagId>
        TDBTagAddress(const TObjectId& objectId, const TTagId& tagId)
            : ObjectId(::ToString(objectId))
            , TagId(::ToString(tagId)) {

        }
    };
}

class ITagDescription {
public:
    enum class EUniquePolicy {
        NonUnique /* "non_unique" */,
        AddIfNotExists /* "add_if_not_exists" */,
        UpsertIfExists /* "upsert_if_exists" */
    };
private:
    CSA_DEFAULT(ITagDescription, TString, Description);
    CSA_MAYBE(ITagDescription, EUniquePolicy, ExplicitUniquePolicy);
public:
    using TPtr = TAtomicSharedPtr<ITagDescription>;
    using TFactory = NObjectFactory::TObjectFactory<ITagDescription, TString>;
    virtual ~ITagDescription() = default;

    virtual EUniquePolicy GetDefaultUniquePolicy() const {
        return EUniquePolicy::NonUnique;
    }

    EUniquePolicy GetUniquePolicy() const {
        return ExplicitUniquePolicy.GetOrElse(GetDefaultUniquePolicy());
    }

    virtual TVector<TDBTagDescription> GetDefaultObjects() const {
        return {};
    }

    virtual TString GetClassName() const = 0;

    virtual NJson::TJsonValue SerializeToJson() const;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info);

    class TDecoder: public TBaseDecoder {
        RTLINE_ACCEPTOR(TDecoder, TagDescriptionObject, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Description, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, ExplicitUniquePolicy, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase) {
            TagDescriptionObject = GetFieldDecodeIndex("tag_description_object", decoderBase);
            Description = GetFieldDecodeIndex("description", decoderBase);
            ExplicitUniquePolicy = GetFieldDecodeIndex("explicit_unique_policy", decoderBase);
        }
    };

    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
    NCS::NStorage::TTableRecord SerializeToTableRecord() const;
    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const;
};

class TDBTagDescription: public TInterfaceContainer<ITagDescription> {
private:
    using TBase = TInterfaceContainer<ITagDescription>;
    CSA_DEFAULT(TDBTagDescription, TString, Name);
    CS_ACCESS(TDBTagDescription, ui32, Revision, 0);
    CS_ACCESS(TDBTagDescription, bool, Deprecated, false);
public:
    using TId = TString;

    TDBTagDescription() = default;
    TDBTagDescription(ITagDescription::TPtr object)
        : TBase(object) {
    }

    TMaybe<ui32> GetRevisionMaybe() const {
        return Revision;
    }

    const TString& GetInternalId() const {
        return Name;
    }

    static TString GetTableName() {
        return "tag_descriptions";
    }

    static TString GetIdFieldName() {
        return "name";
    }

    static TString GetHistoryTableName() {
        return "tag_descriptions_history";
    }

    class TDecoder: public TBase::TDecoder {
        using TBaseDecoder = TInterfaceContainer<ITagDescription>::TDecoder;
        RTLINE_ACCEPTOR(TDecoder, Name, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Revision, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Deprecated, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase)
            : TBaseDecoder(decoderBase) {
            Name = GetFieldDecodeIndex("name", decoderBase);
            Revision = GetFieldDecodeIndex("revision", decoderBase);
            Deprecated = GetFieldDecodeIndex("deprecated", decoderBase);
        }
    };

    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
    NJson::TJsonValue SerializeToJson() const;
    bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    NCS::NStorage::TTableRecord SerializeToTableRecord() const;
    static NFrontend::TScheme GetScheme(const IBaseServer& server);

};

class ITagDescriptions {
public:
    virtual TDBTagDescription GetTagDescription(const TString& name) const = 0;

    template <class T>
    TAtomicSharedPtr<T> GetTagDescriptionAs(const TString& name) const {
        TDBTagDescription dbResult = GetTagDescription(name);
        return dbResult.GetPtrAs<T>();
    }

    static const ITagDescriptions& Instance();

};

class TTagDescriptionsOperator {
private:
    const ITagDescriptions* Description = nullptr;
public:
    void Register(const ITagDescriptions* description);
    void Unregister();
    const ITagDescriptions& GetDescription() const;
};

