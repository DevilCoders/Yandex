#pragma once
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/resources/manager.h>
#include <library/cpp/object_factory/object_factory.h>

namespace NCS {
    namespace NBackground {
        class IRTQueueTask;

        class IRTQueueAction {
        protected:
            virtual bool DoExecute(const IRTQueueTask& task, const IBaseServer& server) const = 0;
            virtual bool DoDeserializeFromBlob(const TBlob& data) = 0;
            virtual TBlob DoSerializeToBlob() const = 0;
        public:
            using TPtr = TAtomicSharedPtr<IRTQueueAction>;
            using TFactory = NObjectFactory::TObjectFactory<IRTQueueAction, TString>;
            virtual ~IRTQueueAction() = default;

            bool Execute(const IRTQueueTask& task, const IBaseServer& server) const noexcept;

            bool DeserializeFromBlob(const TBlob& data) {
                return DoDeserializeFromBlob(data);
            }

            TBlob SerializeToBlob() const {
                return DoSerializeToBlob();
            }

            virtual TString GetClassName() const = 0;
        };

        class TRTQueueActionContainer: public TBaseInterfaceContainer<IRTQueueAction> {
        private:
            using TBase = TBaseInterfaceContainer<IRTQueueAction>;
        public:
            using TBase::TBase;
            TBlob SerializeToBlob() const {
                if (!Object) {
                    return TBlob();
                } else {
                    return Object->SerializeToBlob();
                }
            }
        };

        template <class TProto>
        class IRTQueueActionProto: public IRTQueueAction {
        protected:

            virtual bool DoDeserializeFromProto(const TProto& proto) = 0;
            virtual TProto DoSerializeToProto() const = 0;

            bool DeserializeFromProto(const TProto& proto) {
                return DoDeserializeFromProto(proto);
            }

            TProto SerializeToProto() const {
                return DoSerializeToProto();
            }

            virtual bool DoDeserializeFromBlob(const TBlob& data) override final {
                TProto proto;
                if (!proto.ParseFromArray(data.data(), data.size())) {
                    TFLEventLog::Error("cannot parse proto from blob");
                    return false;
                }
                return DeserializeFromProto(proto);
            }

            virtual TBlob DoSerializeToBlob() const override final {
                TProto proto = SerializeToProto();
                const TString sProto = proto.SerializeAsString();
                return TBlob::Copy(sProto.data(), sProto.size());
            }
        };

        class IRTQueueTask: public TRTQueueActionContainer {
        private:
            using TBase = TRTQueueActionContainer;
            using TTags = TMap<TString, TString>;
            CSA_PROTECTED(IRTQueueTask, TString, InternalTaskId, TGUID::CreateTimebased().AsUuidString());
            CSA_PROTECTED_DEF(IRTQueueTask, TString, OwnerId);
            CSA_PROTECTED_DEF(IRTQueueTask, TString, QueueId);
            CSA_PROTECTED(IRTQueueTask, TInstant, StartInstant, ModelingNow());
            CSA_READONLY_DEF(TTags, Tags);

            virtual void DoFillCustomTags(TMap<TString, TString>& /*result*/) const {
            }

        public:
            using TPtr = TAtomicSharedPtr<IRTQueueTask>;
            virtual ~IRTQueueTask() = default;
            using TBase::TBase;

            TMap<TString, TString> GetCustomTags() const;

            NCS::NLogging::TBaseLogRecord GetLogRecord() const;
            TSignalTagsSet GetSignalTags() const;

            IRTQueueTask& AddTag(const TString& tagId, const TString& tagValue) {
                Tags[tagId] = tagValue;
                return *this;
            }

            IRTQueueTask() = default;
            IRTQueueTask(const IRTQueueTask& base) = default;
        };

        template <class TBaseClass, class TInternalResourceClass>
        class IRTQueueActionWithResource: public TBaseClass {
        protected:
            using TBaseClass::GetClassName;
            virtual TAtomicSharedPtr<TInternalResourceClass> DoExecuteImpl(const IRTQueueTask& task, TAtomicSharedPtr<TInternalResourceClass> state, const IBaseServer& bServer) const = 0;

            virtual TString GetInternalResourceId(const IRTQueueTask& task) const {
                return GetClassName() + ":" + task.GetInternalTaskId();
            }

            virtual bool DoExecute(const IRTQueueTask& task, const IBaseServer& bServer) const override {
                if (!bServer.GetResourcesManager()) {
                    TFLEventLog::Error("no resources manager for task usage");
                    return false;
                }

                const TString dbResourceId = GetInternalResourceId(task);
                TMaybe<NCS::NResources::TDBResource> resource;
                TAtomicSharedPtr<TInternalResourceClass> state;
                {
                    auto session = bServer.GetResourcesManager()->BuildNativeSession(false);
                    if (!bServer.GetResourcesManager()->RestoreByResourceKey(dbResourceId, resource, session)) {
                        TFLEventLog::Error("cannot restore info about resource")("resource_id", dbResourceId);
                        return false;
                    }
                    if (!!resource && !!resource->GetContainer()) {
                        state = resource->GetContainer().GetPtrAs<TInternalResourceClass>();
                        if (!state) {
                            TFLEventLog::Error("incorrect resource class")("db_resource_id", dbResourceId);
                            return false;
                        }
                    }
                }
                TAtomicSharedPtr<TInternalResourceClass> nextState = DoExecuteImpl(task, state, bServer);
                if (!nextState) {
                    TFLEventLog::Error("cannot execute task with resource");
                    return false;
                }
                NCS::NResources::TDBResource dbRes;
                if (!!resource) {
                    dbRes = *resource;
                }
                dbRes.SetKey(dbResourceId);
                dbRes.SetContainer(nextState);
                dbRes.SetDeadline(ModelingNow() + TDuration::Days(1800));

                auto session = bServer.GetResourcesManager()->BuildNativeSession(false);
                if (!bServer.GetResourcesManager()->UpsertObject(dbRes, session) || !session.Commit()) {
                    TFLEventLog::Error("cannot store info about resource")("resource_id", dbResourceId);
                    return false;
                }
                return true;
            }
        public:
        };

        class TRTQueueTaskContainer: public TBaseInterfaceContainer<IRTQueueTask> {
        private:
            using TBase = TBaseInterfaceContainer<IRTQueueTask>;
        public:
            using TBase::TBase;
            NCS::NLogging::TBaseLogRecord GetLogRecord() const {
                NCS::NLogging::TBaseLogRecord result;
                if (!Object) {
                    return result;
                }
                return Object->GetLogRecord();
            }

            TString GetInternalTaskId() const {
                if (!Object) {
                    return "";
                } else {
                    return Object->GetInternalTaskId();
                }
            }

            TString GetOwnerId() const {
                if (!Object) {
                    return "";
                } else {
                    return Object->GetOwnerId();
                }
            }

            TString GetQueueId() const {
                if (!Object) {
                    return "";
                } else {
                    return Object->GetQueueId();
                }
            }

        };

    }
}
