#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/interfaces/container.h>
#include "common.h"

namespace NCS {
    namespace NPropositions {

        class TDBVerdict {
        private:
            CS_ACCESS(TDBVerdict, ui32, VerdictId, 0);
            CS_ACCESS(TDBVerdict, ui32, PropositionId, 0);
            CS_ACCESS(TDBVerdict, ui32, PropositionRevision, 0);
            CSA_DEFAULT(TDBVerdict, TString, Comment);
            CSA_DEFAULT(TDBVerdict, TString, SystemUserId);
            CS_ACCESS(TDBVerdict, TInstant, VerdictInstant, TInstant::Zero());
            CS_ACCESS(TDBVerdict, EVerdict, Verdict, EVerdict::Confirm);
        public:
            bool operator!() const {
                return !SystemUserId;
            }

            using TId = ui32;
            TDBVerdict() = default;

            TId GetInternalId() const {
                return VerdictId;
            }

            static TString GetTableName() {
                return "cs_proposition_verdicts";
            }

            static TString GetIdFieldName() {
                return "verdict_id";
            }

            static TString GetObjectIdFieldName() {
                return "proposition_id";
            }

            static TString GetHistoryTableName() {
                return "cs_proposition_verdicts_history";
            }

            class TDecoder: public TBaseDecoder {
                DECODER_FIELD(VerdictId);
                DECODER_FIELD(PropositionId);
                DECODER_FIELD(PropositionRevision);
                DECODER_FIELD(Comment);
                DECODER_FIELD(SystemUserId);
                DECODER_FIELD(VerdictInstant);
                DECODER_FIELD(Verdict);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase)
                {
                    VerdictId = GetFieldDecodeIndex("verdict_id", decoderBase);
                    PropositionId = GetFieldDecodeIndex("proposition_id", decoderBase);
                    PropositionRevision = GetFieldDecodeIndex("proposition_revision", decoderBase);
                    Comment = GetFieldDecodeIndex("comment", decoderBase);
                    SystemUserId = GetFieldDecodeIndex("system_user_id", decoderBase);
                    Verdict = GetFieldDecodeIndex("verdict", decoderBase);
                    VerdictInstant = GetFieldDecodeIndex("verdict_instant", decoderBase);
                }
            };

            NJson::TJsonValue SerializeToJson() const;
            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            NStorage::TTableRecord SerializeToTableRecord() const;
            Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

            static NFrontend::TScheme GetScheme(const IBaseServer& server);

        };
    }
}

