#include "cursor.h"
#include <kernel/common_server/library/storage/records/db_value.h>
#include <util/stream/zlib.h>

namespace NCS {
    namespace NSelection {
        namespace NSorting {
            const TString ICommonCursor::kDefaultSalt = "hsK**5uGu&a0IZ%E7wE8@jwwSq!";

            NJson::TJsonValue ICommonCursor::SerializeToJson(const TString& salt /*= kDefaultSalt*/) const {
                NJson::TJsonValue result = DoSerializeToJson();
                if (salt) {
                    result.InsertValue("checksum", MD5::Calc(NJson::WriteJson(result, false, true, false) + salt));
                }
                return result;
            }

            bool ICommonCursor::DeserializeFromJson(const NJson::TJsonValue& jsonInfo, const TString& salt /*= kDefaultSalt*/) {
                if (!DoDeserializeFromJson(jsonInfo)) {
                    TFLEventLog::Error("cannot parse cursor implementation");
                    return false;
                }
                if (salt) {
                    NJson::TJsonValue jsonInfoLocal = jsonInfo;
                    const TString strChecksum = jsonInfo["checksum"].GetStringRobust();
                    jsonInfoLocal.EraseValue("checksum");
                    if (strChecksum != MD5::Calc(NJson::WriteJson(jsonInfoLocal, false, true, false) + salt)) {
                        TFLEventLog::Error("incorrect checksum");
                        return false;
                    }
                }
                return true;
            }

            TString ICommonCursor::SerializeToString(const TString& salt /*= kDefaultSalt*/) const {
                NJson::TJsonValue jsonCursor = SerializeToJson(salt);
                TStringStream ss;
                {
                    TZLibCompress zLib(&ss, ZLib::StreamType::GZip);
                    zLib << jsonCursor.GetStringRobust();
                }
                return Base64Encode(ss.Str());
            }

            bool ICommonCursor::DeserializeFromString(const TString& cursorString, const TString& salt /*= kDefaultSalt*/) {
                TString cursorPackedJsonString;
                try {
                    Base64Decode(cursorString, cursorPackedJsonString);
                } catch (...) {
                    TFLEventLog::Error("cannot decode base64 cursor")("raw_data", cursorString);
                    return false;
                }
                TString cursorJsonString;
                try {
                    TStringInput si(cursorPackedJsonString);
                    TZLibDecompress zLib(&si);
                    TStringStream ss;
                    zLib.ReadAll(ss);
                    cursorJsonString = ss.Str();
                } catch (...) {
                    TFLEventLog::Error("cannot unpack cursor")("raw_data", cursorString);
                    return false;
                }
                
                NJson::TJsonValue jsonCursor;
                if (!NJson::ReadJsonFastTree(cursorJsonString, &jsonCursor)) {
                    TFLEventLog::Error("cannot parse cursor as json")("raw_data", cursorJsonString);
                    return false;
                }
                return DeserializeFromJson(jsonCursor, salt);
            }

            NJson::TJsonValue TCursorField::SerializeToJson() const {
                NJson::TJsonValue result = TBase::SerializeToJson();
                result.InsertValue("start_value", NStorage::TDBValueOperator::SerializeToJson(StartValue));
                return result;
            }

            bool TCursorField::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TBase::DeserializeFromJson(jsonInfo)) {
                    return false;
                }
                if (!NStorage::TDBValueOperator::DeserializeFromJson(StartValue, jsonInfo["start_value"])) {
                    return false;
                }
                return true;
            }

            NJson::TJsonValue TSimpleCursor::DoSerializeToJson() const {
                NJson::TJsonValue result = NJson::JSON_MAP;
                result.InsertValue("table_name", TableName);
                TJsonProcessor::WriteObjectsArray(result, "fields", Fields);
                return result;
            }

            bool TSimpleCursor::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TJsonProcessor::Read(jsonInfo, "table_name", TableName)) {
                    return false;
                }
                if (!TJsonProcessor::ReadObjectsContainer(jsonInfo, "fields", Fields)) {
                    return false;
                }
                return true;
            }

            NCS::NSelection::NSorting::TSimpleCursor& TSimpleCursor::FillFromSorting(const TLinear& sorting) {
                TVector<TCursorField> fields;
                for (auto&& i : sorting.GetFields()) {
                    TCursorField cf(i);
                    fields.emplace_back(std::move(cf));
                }
                std::swap(fields, Fields);
                return *this;
            }

            bool TSimpleCursor::FillCursor(const NStorage::TTableRecordWT& record) {
                for (auto&& i : Fields) {
                    TMaybe<NStorage::TDBValue> dbValue = record.GetValue(i.GetFieldId());
                    if (!dbValue) {
                        TFLEventLog::Error("cannot found field for cursor")("field_id", i.GetFieldId());
                        return false;
                    }
                    i.SetStartValue(*dbValue);
                }
                return true;
            }

            void TSimpleCursor::FillSorting(TSRSelect& srSelect) const {
                if (Fields.empty()) {
                    return;
                }
                TSROrder& srOrder = srSelect.RetOrderBy<TSROrder>();
                srOrder.SetFields(Fields);

                auto srMultiOld = srSelect.GetCondition();
                TSRMulti* srMulti = nullptr;
                if (!!srMultiOld) {
                    TSRMulti& srMultiNew = srSelect.RetCondition<TSRMulti>();
                    srMultiNew.MutableNodes().emplace_back(srMultiOld);
                    srMulti = &srMultiNew.RetNode<TSRMulti>(ESRMulti::Or);
                } else {
                    srMulti = &srSelect.RetCondition<TSRMulti>(ESRMulti::Or);
                }
                TSRMulti srMultiConditionEq;
                for (auto&& i : Fields) {
                    auto& srMultiCurrent = srMulti->RetNode<TSRMulti>();
                    srMultiCurrent.MutableNodes() = srMultiConditionEq.GetNodes();
                    srMultiCurrent.InitNode<TSRBinary>(i.GetFieldId(), i.GetStartValue(), i.IsAscending() ? ESRBinary::Greater : ESRBinary::Less);
                    srMultiConditionEq.InitNode<TSRBinary>(i.GetFieldId(), i.GetStartValue());
                }
            }

            TAtomicSharedPtr<NCS::NSelection::NSorting::TLinear> TSimpleCursor::BuildSorting() const {
                TAtomicSharedPtr<TLinear> result = new TLinear;
                for (auto&& i : Fields) {
                    result->RegisterField(i);
                }
                return result;
            }

            NJson::TJsonValue TCompositeCursor::DoSerializeToJson() const {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::WriteObjectsContainerVariadic(result, "cursors", Cursors, "");
                return result;
            }

            bool TCompositeCursor::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TJsonProcessor::ReadObjectsContainerVariadic(jsonInfo, "cursors", Cursors, false, false, true, "")) {
                    return false;
                }
                return true;
            }

            TMaybe<NCS::NSelection::NSorting::TSimpleCursor> TCompositeCursor::GetCursor(const TString& tableName) const {
                for (auto&& i : Cursors) {
                    if (i.GetTableName() == tableName) {
                        return i;
                    }
                }
                return Nothing();
            }

        }
    }
}
