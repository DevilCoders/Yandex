#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/interfaces/container.h>

namespace NCS {
    namespace NResources {
        class IResource {
        protected:
            virtual bool DoOnRemove(const IBaseServer& /*server*/) const {
                return true;
            }
            virtual NCS::NScheme::TScheme DoGetScheme(const IBaseServer& server) const = 0;

        public:
            using TPtr = TAtomicSharedPtr<IResource>;
            using TFactory = NObjectFactory::TObjectFactory<IResource, TString>;
            virtual ~IResource() = default;
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromString(const TString& data) = 0;
            virtual TString SerializeToString() const = 0;

            virtual NJson::TJsonValue SerializeToJson() const = 0;
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;

            virtual TString GetClassName() const = 0;
            virtual bool OnRemove(const IBaseServer& server) const {
                return DoOnRemove(server);
            }
            NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const {
                return DoGetScheme(server);
            }
        };

        template <class TProto>
        class IProtoResource: public INativeProtoSerialization<TProto, IResource> {
        protected:
            virtual bool DoDeserializeFromProto(const TProto& proto) = 0;
            virtual void DoSerializeToProto(TProto& proto) const = 0;
        public:
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromProto(const TProto& proto) override final {
                return DoDeserializeFromProto(proto);
            }

            virtual void SerializeToProto(TProto& proto) const override final {
                DoSerializeToProto(proto);
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromString(const TString& data) override final {
                TProto proto;
                if (!proto.ParseFromArray(data.data(), data.size())) {
                    TFLEventLog::Log("cannot parse proto from string");
                    return false;
                }
                return DeserializeFromProto(proto);
            }
            virtual TString SerializeToString() const override final {
                TProto proto;
                SerializeToProto(proto);
                return proto.SerializeAsString();
            }
        };

        class TResourceContainer: public TBaseInterfaceContainer<IResource> {
        private:
            using TBase = TBaseInterfaceContainer<IResource>;
        public:
            using TBase::TBase;

            TString SerializeToString() const {
                if (!Object) {
                    return "";
                } else {
                    return Object->SerializeToString();
                }
            }
        };

        class TDBResource {
        private:
            CS_ACCESS(TDBResource, ui32, Id, 0);
            CSA_DEFAULT(TDBResource, TString, Key);
            CSA_DEFAULT(TDBResource, TString, AccessId);
            CS_ACCESS(TDBResource, ui32, Revision, 0);
            CS_ACCESS(TDBResource, TInstant, Deadline, TInstant::Zero());
            CSA_DEFAULT(TDBResource, TResourceContainer, Container);
        public:
            using TId = TString;
            TDBResource() = default;

            const TId& GetInternalId() const {
                return Key;
            }

            static TString GetIdFieldName() {
                return "resource_key";
            }

            TMaybe<ui64> GetRevisionMaybe() const {
                return Revision;
            }

            static TString GetTableName() {
                return "cs_resources";
            }

            static TString GetHistoryTableName() {
                return "cs_resources_history";
            }

            class TDecoder: public TBaseDecoder {
                DECODER_FIELD(Id);
                DECODER_FIELD(Key);
                DECODER_FIELD(AccessId);
                DECODER_FIELD(Revision);
                DECODER_FIELD(Deadline);
                DECODER_FIELD(ClassName);
                DECODER_FIELD(Container);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase) {
                    Id = GetFieldDecodeIndex("resource_id", decoderBase);
                    Key = GetFieldDecodeIndex("resource_key", decoderBase);
                    AccessId = GetFieldDecodeIndex("access_id", decoderBase);
                    Revision = GetFieldDecodeIndex("revision", decoderBase);
                    Deadline = GetFieldDecodeIndex("deadline", decoderBase);
                    ClassName = GetFieldDecodeIndex("class_name", decoderBase);
                    Container = GetFieldDecodeIndex("container", decoderBase);
                }
            };

            bool operator !() const {
                return !Key;
            }

            NJson::TJsonValue SerializeToJson() const;
            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            NStorage::TTableRecord SerializeToTableRecord() const;
            Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

            static NFrontend::TScheme GetScheme(const IBaseServer& server);

        };
    }
}

