#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/util/cgi_processing.h>
#include <kernel/common_server/library/storage/records/db_value.h>
#include <kernel/common_server/library/storage/records/record.h>
#include <kernel/common_server/util/algorithm/container.h>
#include <library/cpp/digest/md5/md5.h>
#include "sorting.h"

namespace NCS {
    namespace NSelection {
        namespace NSorting {
            class TCursorField: public NStorage::NRequest::TFieldOrder {
            private:
                using TBase = NStorage::NRequest::TFieldOrder;
                CSA_DEFAULT(TCursorField, NStorage::TDBValue, StartValue);
            public:
                using TBase::TBase;
                TCursorField(const NStorage::NRequest::TFieldOrder& base)
                    : TBase(base)
                {

                }
                NJson::TJsonValue SerializeToJson() const;
                bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            };

            class ICommonCursor {
            protected:
                static const TString kDefaultSalt;
                virtual NJson::TJsonValue DoSerializeToJson() const = 0;
                virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            public:
                virtual ~ICommonCursor() = default;
                NJson::TJsonValue SerializeToJson(const TString& salt = kDefaultSalt) const;
                bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo, const TString& salt = kDefaultSalt);
                TString SerializeToString(const TString& salt = kDefaultSalt) const;
                bool DeserializeFromString(const TString& cursorString, const TString& salt = kDefaultSalt);

            };

            class TSimpleCursor: public ICommonCursor {
            private:
                CSA_READONLY_DEF(TString, TableName);
                CSA_READONLY_DEF(TVector<TCursorField>, Fields);
            protected:
                virtual NJson::TJsonValue DoSerializeToJson() const override;

                virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            public:
                TSimpleCursor() = default;
                TSimpleCursor(const TString& tableName)
                    : TableName(tableName)
                {

                }
                TSimpleCursor& FillFromSorting(const TLinear& sorting);
                bool FillCursor(const NStorage::TTableRecordWT& record);
                void FillSorting(TSRSelect& srSelect) const;
                TAtomicSharedPtr<TLinear> BuildSorting() const;
            };

            class TCompositeCursor: public ICommonCursor {
            private:
                TVector<TSimpleCursor> Cursors;
            protected:
                virtual NJson::TJsonValue DoSerializeToJson() const override;
                virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            public:
                TMaybe<TSimpleCursor> GetCursor(const TString& tableName) const;
                TCompositeCursor& RegisterCursor(const TSimpleCursor& cursor) {
                    Cursors.emplace_back(cursor);
                    return *this;
                }
            };
        }
    }
}
