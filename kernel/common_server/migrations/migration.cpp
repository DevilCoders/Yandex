#include "migration.h"

#include <kernel/common_server/library/logging/events.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <contrib/libs/protobuf/src/google/protobuf/text_format.h>
#include <util/system/filemap.h>
#include <util/generic/strbuf.h>

namespace {
    TString FormantString(const TString& s, size_t len) {
        return s.substr(0, len) + TString(len - Min(s.length(), len), ' ');
    }
}

namespace NCS {
    namespace NStorage {

        void TDBMigrationBase::MergeFrom(const TDBMigrationBase& other) {
            Revision = other.Revision;
        }

        Y_WARN_UNUSED_RESULT bool TDBMigrationBase::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "name", Name, true)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "source", Source)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "db-name", DBName)) {
                return false;
            }
            NProtobufJson::Json2Proto(jsonInfo["header"], Header);
            return true;
        }

        NJson::TJsonValue TDBMigrationBase::SerializeToJson() const noexcept {
            NJson::TJsonValue result;
            TJsonProcessor::Write(result, "revision", Revision);
            TJsonProcessor::Write(result, "source", Source);
            TJsonProcessor::Write(result, "db-name", DBName);
            NProtobufJson::Proto2Json(Header, result["header"]);
            return result;
        }

        NFrontend::TScheme TDBMigrationBase::GetScheme(const IBaseServer& /*server*/) noexcept {
            NFrontend::TScheme result;
            result.Add<TFSString>("name").SetReadOnly(true);
            result.Add<TFSString>("source").SetReadOnly(true);
            result.Add<TFSStructure>("header").SetProto<TMigrationHeader>().SetReadOnly(true);
            result.Add<TFSNumeric>("revision").SetReadOnly(true).SetRequired(false);
            result.Add<TFSString>("db-name").SetReadOnly(true);
            return result;
        }

        bool TDBMigration::TryApply(NCS::NStorage::IDatabase::TPtr dbMigration, TEntitySession& /*session*/, bool markAppliedOnly, const TMigrationHeader* externalHeader) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("TryApplyMigration")("name", Name)("mark_applied", markAppliedOnly);
            TFLEventLog::Info("start");
            TMigrationHeader header = Header;
            if (externalHeader) {
                header.MergeFrom(*externalHeader);
            }
            auto deadline = TDuration::Seconds(header.GetTimeoutS()).ToDeadLine();
            for (ui32 attempt = 0; attempt <= header.GetMaxAttempts(); ++attempt) {
                NCS::TEntitySession sessionLocal(dbMigration->TransactionMaker().ReadOnly(false).SetLockTimeout(TDuration::Zero()).Build());
                if (!markAppliedOnly && (!sessionLocal.Exec(Script) || !sessionLocal.Commit())) {
                    if (Now() > deadline) {
                        TFLEventLog::Error("cannot apply migration")("reason", "deadline")("deadline", deadline);
                        break;
                    } else {
                        TFLEventLog::Error("try migration apply again")("attempt", attempt)("reason", "query failed");
                    }
                } else {
                    AppliedAt = Now();
                    AppliedHash = CalcHash();
                    TFLEventLog::Notice("migration applied");
                    return true;
                }
            };
            TFLEventLog::Error("cannot apply migration")("reason", "attempts")("max_attempts", header.GetMaxAttempts());
            return false;
        }

        TString TDBMigration::CalcHash() const {
            return MD5::Calc(Script);
        }

        TString TDBMigration::GetTableName() {
            return "migrations_history";
        }

        TString TDBMigration::GetIdFieldName() {
            return "filename";
        }

        TString TDBMigration::GetHistoryTableName() {
            return "migrations_history_history";
        }

        const TDBMigration::TId& TDBMigration::GetInternalId() const {
            return Name;
        }

        bool TDBMigration::Apply(NCS::NStorage::IDatabase::TPtr dbMigration, TEntitySession& session, const TMigrationHeader* externalHeader) noexcept {
            return TryApply(dbMigration, session, false, externalHeader);
        }

        bool TDBMigration::MarkApplied(NCS::NStorage::IDatabase::TPtr dbMigration, TEntitySession& session, const TMigrationHeader* externalHeader) noexcept {
            return TryApply(dbMigration, session, true, externalHeader);
        }

        void TDBMigration::ReadFromFile(const TFsPath& path, const TMigrationHeader& defaultHeader) {
            Name = path.GetName();
            TFileMap fileMap(TFile(path, RdOnly));
            fileMap.Map(0, fileMap.GetFile().GetLength());
            TStringBuf content((const char*)fileMap.Ptr(), fileMap.GetFile().GetLength());
            TStringBuf header, script;
            content.RSplit("==\n", header, script);
            Header = defaultHeader;
            google::protobuf::TextFormat::MergeFromString(TString(header), &Header);
            Script = TString(script);
        }

        void TDBMigration::MergeFrom(const TDBMigration& other) {
            TDBMigrationBase::MergeFrom(other);
            AppliedAt = other.GetAppliedAtMaybe();
            AppliedHash = other.GetAppliedHashMaybe();
        }

        bool TDBMigration::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            using TRevisionType = std::remove_reference_t<decltype(*Revision)>;
            auto revision = Max<TRevisionType>();
            READ_DECODER_VALUE_TEMP_OPT(decoder, values, revision, Revision);
            Revision = revision != Max<TRevisionType>() ? TMaybe<TRevisionType>(revision) : Nothing();
            READ_DECODER_VALUE(decoder, values, Name);
            READ_DECODER_VALUE_INSTANT_OPT(decoder, values, AppliedAt);
            TString aHash;
            READ_DECODER_VALUE_TEMP_OPT(decoder, values, aHash, AppliedHash);
            AppliedHash = aHash ? TMaybe<TString>(aHash) : Nothing();
            READ_DECODER_VALUE_OPT(decoder, values, Source);
            return true;
        }

        TTableRecord TDBMigration::SerializeToTableRecord() const {
            TTableRecord result;
            result.Set("filename", Name);
            result.SetNotEmpty("revision", Revision);
            result.SetNotEmpty("apply_ts", AppliedAt);
            result.SetNotEmpty("applied_hash", AppliedHash);
            result.SetNotEmpty("source", Source);
            return result;
        }

        NJson::TJsonValue TDBMigration::SerializeToJson() const noexcept {
            auto result = TDBMigrationBase::SerializeToJson();
            if (Script) {
                TJsonProcessor::Write(result, "force_apply", false);
            }
            TStringBuilder title;
            title << FormantString(GetDBName(), 15) << FormantString(Source, 10) << FormantString(Name, 60) << (AppliedAt ? "applied" : "not applied");
            result.InsertValue("_title", title);
            TJsonProcessor::Write(result, "name", Name);
            TJsonProcessor::Write(result, "mark_applied", false);
            TJsonProcessor::WriteInstant(result, "applied_at", AppliedAt);
            TJsonProcessor::Write(result, "changed", Script && (!AppliedHash || *AppliedHash != CalcHash()));
            TJsonProcessor::Write(result, "id", GetDBName() + "::" + Source + "::" + Name);
            return result;
        }

        NFrontend::TScheme TDBMigration::GetScheme(const IBaseServer& server) noexcept {
            auto result = TDBMigrationBase::GetScheme(server);
            result.Add<TFSBoolean>("force_apply").SetRequired(false);
            result.Add<TFSBoolean>("mark_applied").SetRequired(false);
            result.Add<TFSNumeric>("applied_at").SetVisual(TFSNumeric::EVisualTypes::DateTime).SetReadOnly(true).SetRequired(false);
            result.Add<TFSBoolean>("changed").SetReadOnly(true).SetRequired(false);
            result.Add<TFSString>("_title").SetReadOnly(true);
            result.Add<TFSString>("id").SetReadOnly(true);
            return result;
        }

        Y_WARN_UNUSED_RESULT bool TDBMigrationAction::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TDBMigrationBase::DeserializeFromJson(jsonInfo)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "force_apply", Apply)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "mark_applied", MarkApplied)) {
                return false;
            }
            return true;
        }

        TDBMigration::TDecoder::TDecoder(const TMap<TString, ui32>& decoderBase)
            :TBaseDecoder(false)
        {
            Name = GetFieldDecodeIndex("filename", decoderBase);
            AppliedAt = GetFieldDecodeIndex("apply_ts", decoderBase);
            AppliedHash = GetFieldDecodeIndex("applied_hash", decoderBase);
            Revision = GetFieldDecodeIndex("revision", decoderBase);
            Source = GetFieldDecodeIndex("source", decoderBase);
        }
    }
}
