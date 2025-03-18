#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

namespace NStrm::NPackager::NTemp {
    class TTestTimedMetaWorker: public TRequestWorker {
    public:
        TTestTimedMetaWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        void Work() override;
    };

}
