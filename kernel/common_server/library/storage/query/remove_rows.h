#pragma once
#include "abstract.h"
#include <util/generic/vector.h>
#include <kernel/common_server/util/json_processing.h>

namespace NCS {
    namespace NStorage {

        class TRemoveRowsQuery: public IQuery {
        private:
            CSA_DEFAULT(TRemoveRowsQuery, TString, TableName);
            CSA_DEFAULT(TRemoveRowsQuery, TString, Condition);
            CSA_MUTABLE(TRemoveRowsQuery, bool, NeedReturn, false);
        public:
            TRemoveRowsQuery() = default;
            TRemoveRowsQuery(const TString& tableName)
                : TableName(tableName) {

            }
        };

    }
}
