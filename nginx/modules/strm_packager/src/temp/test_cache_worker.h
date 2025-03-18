#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/base/shm_cache.h>

namespace NStrm::NPackager::NTemp {
    class TTestCacheWorker: public TRequestWorker {
    public:
        TTestCacheWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        void Work() override;

        TShmCache& Cache;
    };

}
