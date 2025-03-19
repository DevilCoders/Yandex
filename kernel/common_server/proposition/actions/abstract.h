#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/library/searchserver/simple/context/replier.h>
#include <kernel/common_server/util/instant_model.h>

namespace NCS {
    namespace NPropositions {

        class IProposedAction {
        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const = 0;

        public:
            virtual ~IProposedAction() = default;
            using TPtr = TAtomicSharedPtr<IProposedAction>;
            using TFactory = NObjectFactory::TObjectFactory<IProposedAction, TString>;
            virtual TString GetClassName() const = 0;
            virtual TString GetObjectId() const = 0;
            virtual TString GetCategoryId() const = 0;
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const = 0;

            virtual TString SerializeToString() const = 0;
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromString(const TString& data) = 0;

            virtual NJson::TJsonValue SerializeToJson() const = 0;
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) = 0;

            virtual TString GetResult() const {
                return "success";
            }

            virtual bool TuneAction(NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const {
                Y_UNUSED(session);
                Y_UNUSED(requestContext);
                return true;
            }

            virtual bool Execute(const TString& userId, const IBaseServer& server) const {
                auto gLogging = TFLRecords::StartContext()("object_id", GetObjectId())("category_id", GetCategoryId())("class_name", GetClassName());
                if (!DoExecute(userId, server)) {
                    TFLEventLog::Error("cannot execute proposition");
                    return false;
                }
                return true;
            }
            virtual bool IsActual(const IBaseServer& server) const = 0;
        };

        template <class TProto>
        class IProtoProposedAction: public IProposedAction, public INativeProtoSerialization<TProto> {
        private:
            using TBaseProtoNative = INativeProtoSerialization<TProto>;
        public:
            virtual TString SerializeToString() const override final {
                TProto proto;
                SerializeToProto(proto);
                return proto.SerializeAsString();
            }
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromString(const TString& data) override final {
                TProto proto;
                if (!proto.ParseFromArray(data.data(), data.size())) {
                    return false;
                }
                return DeserializeFromProto(proto);
            }

            virtual NJson::TJsonValue SerializeToJson() const override final {
                return TBaseProtoNative::SerializeToJson();
            }
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override final {
                return TBaseProtoNative::DeserializeFromJson(jsonData);
            }
        };

        class IJsonProposedAction: public IProposedAction {
        public:
            virtual TString SerializeToString() const override final {
                return SerializeToJson().GetStringRobust();
            }
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromString(const TString& data) override final {
                NJson::TJsonValue jsonInfo;
                if (!ReadJsonFastTree(data, &jsonInfo)) {
                    TFLEventLog::Error("cannot parse json tree from string")("raw_data", data);
                    return false;
                }
                return DeserializeFromJson(jsonInfo);
            }
        };

        class TProposedActionContainerPolicy: public TDefaultInterfaceContainerConfiguration {
        public:
            static TString GetSpecialSectionForType(const TString& /*className*/) {
                return "data";
            }
        };

        class TProposedActionContainer: public TBaseInterfaceContainer<IProposedAction, TProposedActionContainerPolicy> {
        private:
            using TBase = TBaseInterfaceContainer<IProposedAction, TProposedActionContainerPolicy>;
        public:
            using TBase::TBase;
            TString GetObjectId() const {
                if (!Object) {
                    return "";
                }
                return Object->GetObjectId();
            }
            TString GetCategoryId() const {
                if (!Object) {
                    return "";
                }
                return Object->GetCategoryId();
            }
            TString SerializeToString() const {
                if (!Object) {
                    return "";
                }
                return Object->SerializeToString();
            }
            bool Execute(const TString& userId, const IBaseServer& server) const {
                if (!Object) {
                    TFLEventLog::Error("incorrect internal object for execution");
                    return false;
                }
                return Object->Execute(userId, server);
            }
            bool TuneAction(NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const {
                if (!Object) {
                    TFLEventLog::Error("incorrect internal object for PrepareActionForExecute");
                    return false;
                }
                return Object->TuneAction(session, requestContext);
            }
            Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& className, const TString& data) {
                Object = IProposedAction::TFactory::Construct(className);
                if (!Object) {
                    TFLEventLog::Error("cannot construct class instance by name")("class_name", className);
                    return false;
                }
                return Object->DeserializeFromString(data);
            }

            bool IsActual(const IBaseServer& server) const {
                if (!Object) {
                    TFLEventLog::Error("there is no object to check actuality");
                    return false;
                }
                return Object->IsActual(server);
            }
        };

        class IProposedActionWithDelay: public IJsonProposedAction {
            CS_ACCESS(IProposedActionWithDelay, TInstant, ExecuteAt, TInstant::Zero());
        public:
            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result;
                TJsonProcessor::Write(result, "execute_at", ExecuteAt);
                return result;
            }

            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override {
                JREAD_INSTANT_OPT(jsonData, "execute_at", ExecuteAt);
                return true;
            }

            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& /*server*/) const override {
                NFrontend::TScheme scheme;
                scheme.Add<TFSNumeric>("execute_at", "Дата начала времени выполнения (UTC)").IsTimestamp().SetRequired(false);
                return scheme;
            }

            virtual bool IsActual(const IBaseServer& /*server*/) const override {
                return ExecuteAt < ModelingNow();
            }
        };
    }
}

