#pragma once
#include <util/generic/guid.h>
#include <kernel/common_server/util/accessor.h>

namespace NCS {
    namespace NStorage {
        class IQuery {
        private:
            CSA_READONLY(TString, QueryId, TGUID::CreateTimebased().AsGuidString());
        public:
            virtual ~IQuery() = default;
        };
    }
}
