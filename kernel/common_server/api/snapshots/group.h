#pragma once
#include "fetching/abstract/content.h"
#include "removing/abstract/policy.h"
#include "cleaning/abstract/policy.h"
#include "fetching/abstract/context.h"
#include "storage/abstract/storage.h"

namespace NCS {

    class TDBSnapshotsGroup;

    class ISnapshotsComparator {
    public:
        enum class EObjectChangeEvent {
            New,
            Removed,
            Equal
        };
    protected:
        const IBaseServer& Server;
        const TDBSnapshotsGroup& SnapshotsGroup;
        virtual bool DoStart() = 0;
        virtual bool DoFinish() = 0;
    public:
        virtual ~ISnapshotsComparator() = default;

        ISnapshotsComparator(const TDBSnapshotsGroup& g, const IBaseServer& bServer);

        virtual bool OnChangeObject(const NSnapshots::TMappedObject& mObject, const EObjectChangeEvent evType) const = 0;

        bool Start() {
            return DoStart();
        }

        bool Finish() {
            return DoFinish();
        }

        virtual TString BuildObjectId(const NSnapshots::TMappedObject& mObject) const = 0;
    };

    class ISnapshotsDiffPolicy {
    private:
        virtual NJson::TJsonValue DoSerializeToJson() const = 0;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const = 0;
        virtual THolder<ISnapshotsComparator> DoBuildComparator(const TDBSnapshotsGroup& g, const IBaseServer& bServer) const = 0;

        THolder<ISnapshotsComparator> BuildComparator(const TDBSnapshotsGroup& g, const IBaseServer& bServer) const {
            auto result = DoBuildComparator(g, bServer);
            if (!result) {
                TFLEventLog::Error("cannot build comparator");
            }
            return result;
        }
    public:
        using TPtr = TAtomicSharedPtr<ISnapshotsDiffPolicy>;
        using TFactory = NObjectFactory::TObjectFactory<ISnapshotsDiffPolicy, TString>;
        virtual ~ISnapshotsDiffPolicy() = default;
        bool Compare(const TDBSnapshotsGroup& g, const TVector<NSnapshots::TMappedObject>& oldObjects, const TVector<NSnapshots::TMappedObject>& newObjects, const IBaseServer& bServer) const;
        virtual TString GetClassName() const = 0;
        NJson::TJsonValue SerializeToJson() const {
            return DoSerializeToJson();
        }
        bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            return DoDeserializeFromJson(jsonInfo);
        }
        NFrontend::TScheme GetScheme(const IBaseServer& server) const {
            return DoGetScheme(server);
        }
    };

    class TSnapshotsDiffPolicyContainer: public TBaseInterfaceContainer<ISnapshotsDiffPolicy> {
    private:
        using TBase = TBaseInterfaceContainer<ISnapshotsDiffPolicy>;
    public:
        using TBase::TBase;
    };

    class TDBSnapshotsGroup {
    private:
        CSA_DEFAULT(TDBSnapshotsGroup, TString, GroupId);
        CSA_DEFAULT(TDBSnapshotsGroup, TString, DefaultSnapshotCode);
        CS_ACCESS(TDBSnapshotsGroup, ui32, Revision, 0);
        CSA_DEFAULT(TDBSnapshotsGroup, TSnapshotsGroupRemovePolicyContainer, RemovePolicy);
        CSA_DEFAULT(TDBSnapshotsGroup, NSnapshots::TObjectsManagerContainer, ObjectsManager);
        CSA_DEFAULT(TDBSnapshotsGroup, TSnapshotsDiffPolicyContainer, SnapshotsDiffPolicy);
        CSA_DEFAULT(TDBSnapshotsGroup, TSnapshotsGroupCleaningPolicyContainer, CleaningPolicy);
        NJson::TJsonValue SerializeCommonToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            if (!!RemovePolicy) {
                result.InsertValue("remove_policy", RemovePolicy.SerializeToJson());
            }
            if (!!CleaningPolicy) {
                result.InsertValue("cleaning_policy", CleaningPolicy.SerializeToJson());
            }
            result.InsertValue("objects_manager", ObjectsManager.SerializeToJson());
            if (!!SnapshotsDiffPolicy) {
                result.InsertValue("diff_policy", SnapshotsDiffPolicy.SerializeToJson());
            }
            return result;
        }
        bool DeserializeCommonFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::ReadObject(jsonInfo, "remove_policy", RemovePolicy)) {
                return false;
            }
            if (!TJsonProcessor::ReadObject(jsonInfo, "cleaning_policy", CleaningPolicy)) {
                return false;
            }
            if (!TJsonProcessor::ReadObject(jsonInfo, "objects_manager", ObjectsManager, true)) {
                return false;
            }
            if (!ObjectsManager) {
                TFLEventLog::Error("incorrect group objects manager");
                return false;
            }
            if (!TJsonProcessor::ReadObject(jsonInfo, "diff_policy", SnapshotsDiffPolicy)) {
                return false;
            }
            return true;
        }
    public:
        bool operator!() const {
            return !GroupId;
        }

        static TString GetIdFieldName() {
            return "group_id";
        }
        using TId = TString;

        static TString GetTableName() {
            return "snc_groups";
        }

        TString GetInternalId() const {
            return GroupId;
        }

        TMaybe<ui32> GetRevisionMaybe() const {
            return Revision;
        }

        class TDecoder: public TBaseDecoder {
        private:
            using TBase = TBaseDecoder;
            DECODER_FIELD(GroupId);
            DECODER_FIELD(DefaultSnapshotCode);
            DECODER_FIELD(Revision);
            DECODER_FIELD(Data);
        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderBase) {
                GroupId = GetFieldDecodeIndex("group_id", decoderBase);
                DefaultSnapshotCode = GetFieldDecodeIndex("default_snapshot_code", decoderBase);
                Revision = GetFieldDecodeIndex("revision", decoderBase);
                Data = GetFieldDecodeIndex("data", decoderBase);
            }
        };

        bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

        NJson::TJsonValue SerializeToJson() const;
        bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
        NStorage::TTableRecord SerializeToTableRecord() const;
        static NFrontend::TScheme GetScheme(const IBaseServer& server);
    };

}
