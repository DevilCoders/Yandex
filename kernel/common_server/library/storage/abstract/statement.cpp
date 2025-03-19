#include "statement.h"
#include <util/string/strip.h>
#include <library/cpp/charset/ci_string.h>

namespace NCS {
    namespace NStorage {
        TStatement::TStatement(const TString& query, IBaseRecordsSet* records /*= nullptr*/)
            : Query(query)
            , Records(records) {
            Prepare();
        }

        void TStatement::Prepare() {
            StripString(Query, Query);
            StripString(Query, Query, [](const char* c) {
                return c && *c == ';';
                });
            constexpr TStringBuf commit = "COMMIT";
            constexpr TStringBuf rollback = "ROLLBACK";
            const ci_equal_to comparator;
            if (comparator(Query, commit)) {
                Type = EType::Commit;
            } else if (comparator(Query, rollback)) {
                Type = EType::Rollback;
            }
        }

    }
}
