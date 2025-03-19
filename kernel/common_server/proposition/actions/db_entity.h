#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/interfaces/container.h>
#include "abstract.h"

namespace NCS {
    namespace NPropositions {

        template <class TDBObject>
        class TDBEntityAction: public IProposedActionWithDelay {
        private:
            using TBase = IProposedActionWithDelay;
        protected:
            const IDBEntitiesManager<TDBObject>* GetObjectsManager(const IBaseServer& server) const {
                return TDBObject::GetObjectsManager(server);
            }
        public:
            virtual NJson::TJsonValue SerializeToJson() const override {
                return TBase::SerializeToJson();
            }
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override {
                return TBase::DeserializeFromJson(jsonData);
            }
            virtual TString GetCategoryId() const override {
                return TDBObject::GetTableName();
            }
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override {
                return TBase::GetScheme(server);
            }
        };

        template <class TDBObject>
        class TDBEntityRemove: public TDBEntityAction<TDBObject> {
        private:
            using TBase = TDBEntityAction<TDBObject>;
            typename TDBObject::TId ObjectId;
        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const override {
                auto* manager = TBase::GetObjectsManager(server);
                if (!manager) {
                    TFLEventLog::Error("incorrect manager");
                    return false;
                }
                auto session = manager->BuildNativeSession(false);
                if (!manager->RemoveObject({ ObjectId }, userId, session)) {
                    return false;
                }
                if (!session.Commit()) {
                    return false;
                }
                return true;
            }
        public:
            using TBase::TBase;
            TDBEntityRemove(const TDBObject& object)
                : ObjectId(object.GetInternalId())
            {}

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = TBase::SerializeToJson();
                TJsonProcessor::WriteAsString(result, TDBObject::GetIdFieldName(), ObjectId);
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override {
                if (!TBase::DeserializeFromJson(jsonData)) {
                    return false;
                }
                if (!TJsonProcessor::ReadFromString(jsonData, TDBObject::GetIdFieldName(), ObjectId)) {
                    return false;
                }
                return true;
            }

            virtual TString GetObjectId() const override {
                return ::ToString(ObjectId);
            }

            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override {
                NCS::NScheme::TScheme result = TBase::GetScheme(server);
                result.Add<TFSString>(TDBObject::GetIdFieldName());
                return result;
            }

            static TString GetTypeName() {
                return TDBObject::GetTableName() + "__remove";
            }
            virtual TString GetClassName() const override {
                return GetTypeName();
            }
            static TBase::TFactory::TRegistrator<TDBEntityRemove<TDBObject>> Registrator;
        };

        template <class TDBObject>
        class TDBEntityUpsert: public TDBEntityAction<TDBObject> {
        private:
            using TBase = TDBEntityAction<TDBObject>;
            CSA_DEFAULT(TDBEntityUpsert, TDBObject, Object);

        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const override {
                auto* manager = TBase::GetObjectsManager(server);
                if (!manager) {
                    TFLEventLog::Error("incorrect manager");
                    return false;
                }
                auto session = manager->BuildNativeSession(false);
                if (!manager->UpsertObject(Object, userId, session)) {
                    return false;
                }
                if (!session.Commit()) {
                    return false;
                }
                return true;
            }
        public:
            using TBase::TBase;
            TDBEntityUpsert(const TDBObject& object)
                : Object(object)
            {}

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = TBase::SerializeToJson();
                TJsonProcessor::WriteObject(result, "object", Object);
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override {
                if (!TBase::DeserializeFromJson(jsonData)) {
                    return false;
                }
                if (!TJsonProcessor::ReadObject(jsonData, "object", Object)) {
                    return false;
                }
                return true;
            }

            virtual TString GetObjectId() const override {
                return ::ToString(Object.GetInternalId());
            }

            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override {
                NCS::NScheme::TScheme result = TBase::GetScheme(server);
                result.Add<TFSStructure>("object").SetStructure(TDBObject::GetScheme(server));
                return result;
            }

            static TString GetTypeName() {
                return TDBObject::GetTableName() + "__upsert";
            }
            virtual TString GetClassName() const override {
                return GetTypeName();
            }
            static TBase::TFactory::TRegistrator<TDBEntityUpsert<TDBObject>> Registrator;
        };

        template <class TDBObject>
        IProposedAction::TFactory::TRegistrator<TDBEntityUpsert<TDBObject>> TDBEntityUpsert<TDBObject>::Registrator(TDBEntityUpsert<TDBObject>::GetTypeName());

        template <class TDBObject>
        IProposedAction::TFactory::TRegistrator<TDBEntityRemove<TDBObject>> TDBEntityRemove<TDBObject>::Registrator(TDBEntityRemove<TDBObject>::GetTypeName());

    }
}

