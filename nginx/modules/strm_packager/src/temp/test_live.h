#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

namespace NStrm::NPackager::NTemp {
    class TLiveTestWorker: public TRequestWorker {
    public:
        static void CheckConfig(const TLocationConfig& config);

        TLiveTestWorker(TRequestContext& context, const TLocationConfig& config);

    private:
        void Work() override;
    };

}
