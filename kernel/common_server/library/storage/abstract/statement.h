#pragma once
#include <kernel/common_server/library/storage/records/abstract.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NCS {
    namespace NStorage {
        class TStatement {
        public:
            enum class EType {
                Generic,
                Commit,
                Rollback,
            };

        public:

            using TPtr = TAtomicSharedPtr<TStatement>;

            TStatement(const TString& query, IBaseRecordsSet* records = nullptr);
            TStatement(const TString& query, IBaseRecordsSet& records)
                : TStatement(query, &records) {
            }

            ~TStatement() = default;

            const TString& GetQuery() const {
                return Query;
            }
            IBaseRecordsSet* GetRecords() const {
                return Records;
            }
            EType GetType() const {
                return Type;
            }

        private:
            void Prepare();

        private:
            TString Query;
            IBaseRecordsSet* Records = nullptr;
            EType Type = EType::Generic;
        };
    }
}
