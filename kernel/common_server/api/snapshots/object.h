#pragma once
#include "fetching/abstract/content.h"
#include "fetching/abstract/context.h"
#include "storage/abstract/storage.h"

namespace NCS {

    class ISnapshotContext {
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const = 0;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
    public:
        using TFactory = NObjectFactory::TObjectFactory<ISnapshotContext, TString>;
        using TPtr = TAtomicSharedPtr<ISnapshotContext>;
        virtual ~ISnapshotContext() = default;
        virtual NJson::TJsonValue SerializeToJson() const {
            return DoSerializeToJson();
        }
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            return DoDeserializeFromJson(jsonInfo);
        }
        virtual TString GetClassName() const = 0;
    };

    class TSnapshotContextContainerConfiguration: public TDefaultInterfaceContainerConfiguration {
    public:
        static TString GetSpecialSectionForType(const TString& /*className*/) {
            return "data";
        }
    };

    class TSnapshotContextContainer: public TBaseInterfaceContainer<ISnapshotContext, TSnapshotContextContainerConfiguration> {
    private:
        using TBase = TBaseInterfaceContainer<ISnapshotContext, TSnapshotContextContainerConfiguration>;
    public:
        using TBase::TBase;
    };

    class TDBSnapshot {
    public:
        enum class ESnapshotStatus {
            Undefined = 1,
            InConstruction,
            Ready,
            Removing,
            Removed,
        };
    private:
        CSA_DEFAULT(TDBSnapshot, TString, SnapshotCode);
        CS_ACCESS(TDBSnapshot, ui32, SnapshotId, 0);
        CSA_DEFAULT(TDBSnapshot, TString, SnapshotGroupId);
        CS_ACCESS(TDBSnapshot, ui32, Revision, 0);
        CS_ACCESS(TDBSnapshot, bool, Enabled, false);
        CS_ACCESS(TDBSnapshot, ESnapshotStatus, Status, ESnapshotStatus::Undefined);
        CS_ACCESS(TDBSnapshot, TInstant, LastStatusModification, TInstant::Zero());
        CSA_DEFAULT(TDBSnapshot, TSnapshotContentFetcher, ContentFetcher);
        CSA_DEFAULT(TDBSnapshot, TSnapshotContentFetcherContext, FetchingContext);
        CSA_DEFAULT(TDBSnapshot, TSnapshotContextContainer, SnapshotContext);
    public:
        bool IsReady() const {
            return Enabled && Status == ESnapshotStatus::Ready;
        }

        bool operator!() const {
            return !SnapshotCode;
        }

        using TId = TString;

        static TString GetTableName() {
            return "snc_snapshots";
        }

        static TString GetIdFieldName() {
            return "snapshot_code";
        }

        TId GetInternalId() const {
            return SnapshotCode;
        }

        TMaybe<ui32> GetRevisionMaybe() const {
            return Revision;
        }

        class TDecoder: public TBaseDecoder {
        private:
            using TBase = TBaseDecoder;
            DECODER_FIELD(SnapshotCode);
            DECODER_FIELD(SnapshotId);
            DECODER_FIELD(SnapshotGroupId);
            DECODER_FIELD(Revision);
            DECODER_FIELD(Enabled);
            DECODER_FIELD(Status);
            DECODER_FIELD(LastStatusModification);
            DECODER_FIELD(FetchingContext);
            DECODER_FIELD(ContentFetcher);
            DECODER_FIELD(SnapshotContext);
        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderBase) {
                SnapshotCode = GetFieldDecodeIndex("snapshot_code", decoderBase);
                SnapshotId = GetFieldDecodeIndex("snapshot_id", decoderBase);
                SnapshotGroupId = GetFieldDecodeIndex("snapshot_group_id", decoderBase);
                Revision = GetFieldDecodeIndex("revision", decoderBase);
                Enabled = GetFieldDecodeIndex("enabled", decoderBase);
                Status = GetFieldDecodeIndex("status", decoderBase);
                LastStatusModification = GetFieldDecodeIndex("last_status_modification", decoderBase);
                FetchingContext = GetFieldDecodeIndex("fetching_context", decoderBase);
                ContentFetcher = GetFieldDecodeIndex("content_fetcher", decoderBase);
                SnapshotContext = GetFieldDecodeIndex("snapshot_context", decoderBase);
            }
        };

        bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
        NStorage::TTableRecord SerializeToTableRecord() const;
        static NFrontend::TScheme GetScheme(const IBaseServer& server);
        NJson::TJsonValue SerializeToJson() const;
    };

}
