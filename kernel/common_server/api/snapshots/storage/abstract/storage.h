#pragma once
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/library/storage/records/t_record.h>
#include <kernel/common_server/api/snapshots/objects/mapped/object.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include "structure.h"
#include <kernel/common_server/library/scheme/scheme.h>

namespace NCS {
    namespace NSnapshots {
        class ISelectionResult {
        protected:
            virtual bool DoFetch(TVector<TMappedObject>& result) const = 0;
        public:
            using TPtr = TAtomicSharedPtr<ISelectionResult>;
            virtual ~ISelectionResult() = default;
            bool Fetch(TVector<TMappedObject>& result) const noexcept {
                try {
                    return DoFetch(result);
                } catch (...) {
                    TFLEventLog::Error("cannot fetch data through exception")("exception", CurrentExceptionMessage());
                    return false;
                }
            }
        };

        class TEmptySelectionResult: public ISelectionResult {
        protected:
            virtual bool DoFetch(TVector<TMappedObject>& result) const override {
                result.clear();
                return true;
            }
        public:
        };

        class TObjectsFilter {
        private:
            CSA_MAYBE(TObjectsFilter, ui32, Limit);
            CSA_MAYBE(TObjectsFilter, ui32, Shift);
        public:
            static NCS::NScheme::TScheme GetScheme() {
                NCS::NScheme::TScheme result;
                result.Add<TFSNumeric>("limit").SetDefault(100);
                result.Add<TFSNumeric>("shift").SetDefault(0);
                return result;
            }

            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TJsonProcessor::Read(jsonInfo, "limit", Limit)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "shift", Shift)) {
                    return false;
                }
                return true;
            }
        };

        class IObjectsManager {
        private:
            CSA_DEFAULT(IObjectsManager, TStructure, Structure);
            TAtomic Active = 0;
        protected:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual NJson::TJsonValue DoSerializeToJson() const = 0;
            virtual bool DoRemoveSnapshot(const ui32 snapshotId) const = 0;
            virtual bool DoCreateSnapshot(const ui32 snapshotId) const = 0;
            virtual bool DoRemoveSnapshotObjects(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const = 0;
            virtual bool DoRemoveSnapshotObjectsBySRCondition(const ui32 snapshotId, const TSRCondition& customCondition) const = 0;
            virtual ISelectionResult::TPtr DoGetObjects(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const = 0;
            virtual bool DoGetAllObjects(const ui32 snapshotId, TVector<TMappedObject>& result, const TObjectsFilter& objectsFilter) const = 0;
            virtual bool DoPutObjects(const TVector<TMappedObject>& result, const ui32 snapshotId, const TString& userId) const = 0;
            virtual bool DoUpsertObjects(const TIndex& index, const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& userId) const = 0;
            virtual bool DoInitialize(const IBaseServer& server) = 0;
        public:
            using TFactory = NObjectFactory::TObjectFactory<IObjectsManager, TString>;
            using TPtr = TAtomicSharedPtr<IObjectsManager>;
            virtual ~IObjectsManager() = default;
            virtual TString GetClassName() const = 0;
            bool IsActive() const;
            bool Initialize(const IBaseServer& server);
            bool RemoveSnapshot(const ui32 snapshotId) const;
            bool CreateSnapshot(const ui32 snapshotId) const;
            bool RemoveSnapshotObjects(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const;
            bool RemoveSnapshotObjectsBySRCondition(const ui32 snapshotId, const TSRCondition& customCondition) const;
            bool GetObjects(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId, TVector<TMappedObject>& result) const;

            bool GetAllObjects(const ui32 snapshotId, TVector<TMappedObject>& result, const TObjectsFilter& objectsFilter = Default<TObjectsFilter>()) const;
            template <class TObject>
            bool GetObjects(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId, TVector<TObject>& result) const {
                TVector<TMappedObject> resultMapped;
                if (!GetObjects(objectIds, snapshotId, resultMapped)) {
                    return false;
                }
                TVector<TObject> resultLocal;
                for (auto&& i : resultMapped) {
                    TObject obj;
                    if (!TBaseDecoder::DeserializeFromTableRecordCommon(obj, i.SerializeToTableRecord().BuildWT(), false)) {
                        TFLEventLog::Error("cannot deserialize object")("table_record", i.SerializeToTableRecord());
                        return false;
                    }
                    resultLocal.emplace_back(std::move(obj));
                }
                std::swap(resultLocal, result);
                return true;
            }
            virtual ISelectionResult::TPtr FetchObjectAsync(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const;

            template <class TObject>
            bool PutObjects(const TVector<TObject>& objects, const ui32 snapshotId, const TString& userId) const {
                TVector<TMappedObject> objectsMapped;
                for (auto&& i : objects) {
                    objectsMapped.emplace_back(i.SerializeToTableRecord());
                }
                return PutObjects(objectsMapped, snapshotId, userId);
            }

            template <class TObject>
            bool UpsertObjects(const TVector<TObject>& objects, const ui32 snapshotId, const TString& userId) const {
                TVector<TMappedObject> objectsMapped;
                for (auto&& i : objects) {
                    objectsMapped.emplace_back(i.SerializeToTableRecord());
                }
                return UpsertObjects(objectsMapped, snapshotId, userId);
            }

            bool PutObjects(const TVector<TMappedObject>& result, const ui32 snapshotId, const TString& userId) const;
            bool UpsertObjects(const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& userId) const;

            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);

            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = DoSerializeToJson();
                TJsonProcessor::WriteObject(result, "structure", Structure);
                return result;
            }

            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const {
                NCS::NScheme::TScheme result;
                result.Add<TFSStructure>("structure").SetStructure(TStructure::GetScheme(server));
                return result;
            }
        };

        class TObjectsManagerContainer: public TBaseInterfaceContainer<IObjectsManager> {
        private:
            using TBase = TBaseInterfaceContainer<IObjectsManager>;
        public:
            using TBase::TBase;
            bool Start(const IBaseServer& server);
        };
    }
}
