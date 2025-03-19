#pragma once
#include <kernel/common_server/library/storage/reply/decoder.h>
#include <library/cpp/yson/node/node.h>
#include <kernel/common_server/api/snapshots/storage/abstract/structure.h>
#include <kernel/common_server/library/storage/records/t_record.h>

namespace NCS {
    namespace NSnapshots {
        class TMappedObject {
        private:
            CSA_DEFAULT(TMappedObject, NStorage::TTableRecordWT, Values);
        public:

            static NCS::NScheme::TScheme GetScheme(const IBaseServer& server);

            TMappedObject() = default;
            TMappedObject(const NStorage::TTableRecord& tRecord);
            TMappedObject(const NStorage::TTableRecordWT& tRecord);
            using TDecoder = NCS::NStorage::TSimpleDecoder;

            void FilterColumns(const TSet<TString>& fieldIds);
            Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
            NStorage::TTableRecord SerializeToTableRecord() const;

            Y_WARN_UNUSED_RESULT bool DeserializeFromYTNode(const NYT::TNode& row, const TMap<TString, TString>& factorNamesRemapping, const TStructure& structure);

            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = NJson::JSON_MAP;
                result.InsertValue("values", Values.SerializeToJson());
                return result;
            }
            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                return Values.DeserializeFromJson(jsonInfo["values"]);
            }

            static TString GetTypeName() {
                return "mapped_object";
            }
        };
    }
}
