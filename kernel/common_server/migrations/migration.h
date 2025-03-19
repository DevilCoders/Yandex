#pragma once

#include <kernel/common_server/api/common.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/storage/abstract/database.h>
#include <kernel/common_server/library/storage/records/record.h>
#include <kernel/common_server/library/storage/reply/decoder.h>
#include <kernel/common_server/proto/migration.pb.h>
#include <kernel/common_server/util/accessor.h>

#include <util/datetime/base.h>

class IBaseServer;

namespace NCS {
    namespace NStorage {
        class TDBMigrationBase : public TAtomicRefCount<TDBMigrationBase> {
        protected:
            CSA_MAYBE_PROTECTED_EXCEPT(TDBMigrationBase, ui32, Revision);
            CSA_READONLY_PROTECTED_DEF(TString, Name);
            CSA_READONLY_PROTECTED_DEF(TMigrationHeader, Header);
            CSA_PROTECTED_DEF(TDBMigrationBase, TString, Source);
            CSA_DEFAULT(TDBMigrationBase, TString, DBName);

        public:
            virtual ~TDBMigrationBase() = default;
            void MergeFrom(const TDBMigrationBase& other);
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            NJson::TJsonValue SerializeToJson() const noexcept;
            static NFrontend::TScheme GetScheme(const IBaseServer& server) noexcept;
        };

        class TDBMigration: public TDBMigrationBase {
        public:
            using TPtr = TIntrusivePtr<TDBMigration>;
            using TId = decltype(Name);
            class TDecoder : public TBaseDecoder {
                DECODER_FIELD(Name);
                DECODER_FIELD(Source);
                DECODER_FIELD(AppliedAt);
                DECODER_FIELD(AppliedHash);
                DECODER_FIELD(Revision);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase);
            };

            static TString GetTableName();
            static TString GetIdFieldName();
            static TString GetHistoryTableName();
            const TId& GetInternalId() const;
            bool operator !() const {
                return !Name;
            }

            CSA_READONLY_DEF(TString, Script);
            CSA_READONLY_MAYBE_EXCEPT(TInstant, AppliedAt);
            CSA_READONLY_MAYBE_EXCEPT(TString, AppliedHash);
        public:
            bool Apply(NCS::NStorage::IDatabase::TPtr dbMigration, TEntitySession& session, const TMigrationHeader* externalHeader = nullptr) noexcept;
            bool MarkApplied(NCS::NStorage::IDatabase::TPtr dbMigration, TEntitySession& session, const TMigrationHeader* externalHeader = nullptr) noexcept;
            void ReadFromFile(const TFsPath& path, const TMigrationHeader& defaultHeader);
            void MergeFrom(const TDBMigration& other);
            bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
            TTableRecord SerializeToTableRecord() const;
            NJson::TJsonValue SerializeToJson() const noexcept;
            static NFrontend::TScheme GetScheme(const IBaseServer& server) noexcept;
        private:
            bool TryApply(NCS::NStorage::IDatabase::TPtr dbMigration, TEntitySession& session, bool markAppliedOnly, const TMigrationHeader* externalHeader) noexcept;
            TString CalcHash() const;
        };

        class TDBMigrationAction: public TDBMigrationBase {
            CSA_READONLY(bool, Apply, false);
            CSA_READONLY(bool, MarkApplied, false);
        public:
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        };
    }
}
