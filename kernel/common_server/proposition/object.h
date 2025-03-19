#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/interfaces/container.h>
#include "actions/abstract.h"
#include "policies/abstract.h"

namespace NCS {
    namespace NPropositions {

        class TDBProposition {
        private:
            CS_ACCESS(TDBProposition, ui32, PropositionId, 0);
            CSA_DEFAULT(TDBProposition, TString, PropositionObjectId);
            CSA_DEFAULT(TDBProposition, TString, PropositionCategoryId);
            CS_ACCESS(TDBProposition, ui32, Revision, 0);
            CSA_READONLY_DEF(TProposedActionContainer, ProposedObject);
            CSA_READONLY_DEF(TPropositionPolicyContainer, PropositionPolicy);
            TDBProposition& SetProposedObject(const TProposedActionContainer& container);
        public:

            static TString GetTypeName() {
                return "proposition";
            }

            bool operator!() const {
                return !ProposedObject;
            }

            using TId = ui32;
            TDBProposition() = default;
            TDBProposition(const TProposedActionContainer& container) {
                SetProposedObject(container);
            }

            static TString GetIdFieldName() {
                return "proposition_id";
            }
            TMaybe<ui32> GetRevisionMaybe() const {
                return Revision;
            }

            TId GetInternalId() const {
                return PropositionId;
            }

            TString GetTitle() const;

            static TString GetTableName() {
                return "cs_propositions";
            }

            static TString GetHistoryTableName() {
                return "cs_propositions_history";
            }

            class TDecoder: public TBaseDecoder {
                DECODER_FIELD(PropositionId);
                DECODER_FIELD(PropositionObjectId);
                DECODER_FIELD(PropositionCategoryId);
                DECODER_FIELD(Revision);
                DECODER_FIELD(ClassName);
                DECODER_FIELD(Data);
                DECODER_FIELD(PropositionPolicy);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase)
                {
                    PropositionId = GetFieldDecodeIndex("proposition_id", decoderBase);
                    PropositionObjectId = GetFieldDecodeIndex("proposition_object_id", decoderBase);
                    PropositionCategoryId = GetFieldDecodeIndex("proposition_category_id", decoderBase);
                    Revision = GetFieldDecodeIndex("revision", decoderBase);
                    ClassName = GetFieldDecodeIndex("class_name", decoderBase);
                    Data = GetFieldDecodeIndex("data", decoderBase);
                    PropositionPolicy = GetFieldDecodeIndex("proposition_policy", decoderBase);
                }
            };

            NJson::TJsonValue SerializeToJson() const;
            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            NStorage::TTableRecord SerializeToTableRecord() const;
            Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

            static NFrontend::TScheme GetScheme(const IBaseServer& server);

            bool CheckPolicy(const IBaseServer& server) const;
        };
    }
}

