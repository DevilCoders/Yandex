#pragma once
#include <util/generic/ptr.h>
#include "abstract/abstract.h"
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/library/logging/events.h>

class TNotifierContainer {
private:
    IFrontendNotifierConfig::TPtr NotifierConfig;
    IFrontendNotifier::TPtr Notifier;

    RTLINE_ACCEPTOR_DEF(TNotifierContainer, Name, TString);
    RTLINE_READONLY_ACCEPTOR(Revision, ui64, Max<ui64>());
public:
    using TId = TString;
    const TString& GetInternalId() const {
        return Name;
    }

    TNotifierContainer() = default;

    TNotifierContainer(IFrontendNotifierConfig::TPtr config)
        : NotifierConfig(config)
        , Notifier(config->Construct())
    {
    }

    bool HasRevision() const {
        return Revision != Max<ui64>();
    }

    TMaybe<ui64> GetRevisionMaybe() const {
        return HasRevision() ? Revision : TMaybe<ui64>();
    }

    class TDecoder: public TBaseDecoder {
    public:
        RTLINE_ACCEPTOR(TDecoder, Revision, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Name, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Type, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Meta, i32, -1);

    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase, const bool strict = true)
            : TBaseDecoder(strict)
        {
            Revision = GetFieldDecodeIndex("revision", decoderBase);
            Name = GetFieldDecodeIndex("name", decoderBase);
            Meta = GetFieldDecodeIndex("meta", decoderBase);
            Type = GetFieldDecodeIndex("type", decoderBase);
        }

        static bool NeedVerboseParsingErrorLogging() {
            return false;
        }
    };

    static NFrontend::TScheme GetScheme(const IBaseServer& server);

    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

    NJson::TJsonValue GetReport() const;

    NStorage::TTableRecord SerializeToTableRecord() const;
    NJson::TJsonValue SerializeToJson() const;

    TAtomicSharedPtr<IFrontendNotifier> GetNotifierPtr() const {
        return Notifier;
    }

    ~TNotifierContainer() = default;

    const IFrontendNotifier* operator->() const {
        return Notifier.Get();
    }

    bool operator!() const {
        return !NotifierConfig || !Notifier;
    }

    static TString GetHistoryTableName() {
        return "notifiers_history";
    }

    static TString GetIdFieldName() {
        return "name";
    }

    static TString GetTableName() {
        return "notifiers";
    }

    void Start(const IBaseServer& server);

    TNotifierContainer DeepCopy() const {
        TNotifierContainer result;
        NCS::NStorage::TTableRecordWT wtRecord = SerializeToTableRecord().BuildWT();
        if (!TBaseDecoder::DeserializeFromTableRecordStrictable(result, wtRecord, true)) {
            TFLEventLog::Log("Incorrect deep copy for notifier", TLOG_ERR)("name", GetName());
            Y_ASSERT(false);
        }
        return result;
    }

};

class TNotifiersManagerConfig: public TDBEntitiesManagerConfig {
public:
    using TDBEntitiesManagerConfig::TDBEntitiesManagerConfig;
};

class TNotifiersManager: public TDBEntitiesManager<TNotifierContainer> {
private:
    using TBase = TDBEntitiesManager<TNotifierContainer>;
    const IBaseServer& Server;

public:
    TNotifiersManager(const IBaseServer& server, THolder<IHistoryContext>&& context,
                      const TNotifiersManagerConfig& config)
        : TBase(std::move(context), config)
        , Server(server)
    {
    }

protected:
    virtual TNotifierContainer PrepareForUsage(const TNotifierContainer& evHistory) const override {
        TNotifierContainer result = evHistory.DeepCopy();
        result.Start(Server);
        return result;
    }
public:
    using TBase::TBase;
};
