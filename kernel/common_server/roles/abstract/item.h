#pragma once
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/abstract/frontend.h>

class IItemPermissions: public IJsonDecoderSerializable {
private:
    using TBase = IJsonDecoderSerializable;
    CS_ACCESS(IItemPermissions, i32, Priority, 0);
    CS_ACCESS(IItemPermissions, bool, Enabled, true);
public:
    bool IsEnabled() const {
        return Enabled;
    }

    virtual ~IItemPermissions() = default;

    using TFactory = NObjectFactory::TObjectFactory<IItemPermissions, TString>;
    using TPtr = TAtomicSharedPtr<IItemPermissions>;

    IItemPermissions() = default;

    class TDecoder: public TBase::TDecoder {
        RTLINE_ACCEPTOR(TDecoder, Priority, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Enabled, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase)
            : IJsonDecoderSerializable::TDecoder(decoderBase)
        {
            Priority = GetFieldDecodeIndex("item_priority", decoderBase);
            Enabled = GetFieldDecodeIndex("item_enabled", decoderBase);
        }
    };

    NStorage::TTableRecord SerializeToTableRecord() const {
        NStorage::TTableRecord result = TBase::SerializeToTableRecord();
        result.Set("item_priority", Priority);
        result.Set("item_enabled", Enabled);
        return result;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        if (!TBase::DeserializeWithDecoder(decoder, values)) {
            return false;
        }
        READ_DECODER_VALUE(decoder, values, Priority);
        READ_DECODER_VALUE(decoder, values, Enabled);
        return true;
    }

    virtual TString GetClassName() const = 0;

    virtual NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        result.InsertValue("priority", Priority);
        result.InsertValue("item_enabled", Enabled);
        return result;
    }

    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const final {
        NFrontend::TScheme scheme = DoGetScheme(server);
        scheme.Add<TFSBoolean>("item_enabled").SetDefault(true);
        scheme.Add<TFSNumeric>("priority");
        return scheme;
    }

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) {
        if (!TJsonProcessor::Read(info, "priority", Priority)) {
            return false;
        }
        if (!TJsonProcessor::Read(info, "item_enabled", Enabled)) {
            return false;
        }
        return true;
    }

    virtual void Init(const TYandexConfig::Section* section) final;

    virtual void ToString(IOutputStream& os) const final;

protected:
    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const {
        return NFrontend::TScheme();
    };
};

class TItemPermissionContainerConfiguration: public TDefaultInterfaceContainerConfiguration {
public:
    static TString GetSpecialSectionForType(const TString& /*className*/) {
        return "data";
    }
};

class TItemPermissionContainer: public TInterfaceContainer<IItemPermissions, TItemPermissionContainerConfiguration> {
private:
    using TBase = TInterfaceContainer<IItemPermissions, TItemPermissionContainerConfiguration>;
    CSA_PROTECTED_DEF(TItemPermissionContainer, TString, ItemId);
    CSA_PROTECTED(TItemPermissionContainer, ui32, Revision, 0);
public:
    using TBase::TBase;
    NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result = TBase::SerializeToJson();
        TJsonProcessor::Write(result, "item_id", ItemId);
        TJsonProcessor::Write(result, "revision", Revision);
        return result;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TBase::DeserializeFromJson(jsonInfo)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "item_id", ItemId, true)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
            return false;
        }
        return true;
    }

    static NFrontend::TScheme GetScheme(const IBaseServer& server) {
        auto result = TBase::GetScheme(server);
        result.Add<TFSString>("item_id");
        result.Add<TFSNumeric>("revision").SetReadOnly(true);
        return result;
    }
};

