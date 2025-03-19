#pragma once
#include "obfuscators/abstract.h"
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/util/json_processing.h>

namespace NCS {
    namespace NObfuscator {

        class TDBObfuscator: public TObfuscatorContainer {
        private:
            using TBase = TObfuscatorContainer;
            CS_ACCESS(TDBObfuscator, ui32, ObfuscatorId, 0);
            CS_ACCESS(TDBObfuscator, ui32, Revision, 0);
            CSA_DEFAULT(TDBObfuscator, TString, Name);
            CSA_DEFAULT(TDBObfuscator, ui32, Priority);

        public:
            using TId = ui32;
            TDBObfuscator() = default;

            TMaybe<ui32> GetRevisionMaybe() const {
                return Revision;
            }

            TString GetPublicObjectId() const {
                return Name;
            }

            TId GetInternalId() const {
                return ObfuscatorId;
            }

            static TString GetTypeName() {
                return "obfuscator";
            }

            static TString GetIdFieldName() {
                return "obfuscator_id";
            }

            static TString GetTableName() {
                return "cs_obfuscators";
            }

            static TString GetHistoryTableName() {
                return "cs_obfuscators_history";
            }

            class TDecoder: public TBase::TDecoder {
            private:
                using TBase = TObfuscatorContainer::TDecoder;
                DECODER_FIELD(ObfuscatorId);
                DECODER_FIELD(Revision);
                DECODER_FIELD(Name);
                DECODER_FIELD(Priority);

            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase)
                    : TBase(decoderBase)
                {
                    ObfuscatorId = GetFieldDecodeIndex("obfuscator_id", decoderBase);
                    Revision = GetFieldDecodeIndex("revision", decoderBase);
                    Name = GetFieldDecodeIndex("name", decoderBase);
                    Priority = GetFieldDecodeIndex("priority", decoderBase);
                }
            };

            NJson::TJsonValue SerializeToJson() const;
            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
            NStorage::TTableRecord SerializeToTableRecord() const;
            static NFrontend::TScheme GetScheme(const IBaseServer& server);
        };
    }
}
