#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

#include <util/generic/map.h>

namespace NStrm::NPackager::NTemp {
    class TTestReadWorker: public TRequestWorker {
    public:
        TTestReadWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        void Work() override;
    };

}
