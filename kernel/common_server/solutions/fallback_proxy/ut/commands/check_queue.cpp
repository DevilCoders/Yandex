#include "check_queue.h"
#include <kernel/common_server/library/storage/query/request.h>

namespace NCS {
    namespace NFallbackProxy {

        bool TCheckFallbackQueue::Check(NServerTest::ITestContext& context) const {
            auto db = context.GetServer<IBaseServer>().GetDatabase("main-db");
            TRecordsSetWT records;
            TSRSelect srSelect("cs_pq_pq", &records);
            CHECK_WITH_LOG(db->CreateTransaction(true)->ExecRequest(srSelect)->IsSucceed());
            if (ExpectedSize && *ExpectedSize != records.size()) {
                return false;
            }
            return true;
        }

    }
}
