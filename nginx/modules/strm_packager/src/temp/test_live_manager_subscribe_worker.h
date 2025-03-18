#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/base/live_manager.h>

namespace NStrm::NPackager::NTemp {
    class TTestLiveManagerSubscribeWorker: public TRequestWorker {
    public:
        TTestLiveManagerSubscribeWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        void Work() override;
    };

}
