#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

namespace NStrm::NPackager::NTemp {
    class TMP4VodTestWorker: public TRequestWorker {
    public:
        TMP4VodTestWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        void Work() override;

        void TestUnion();
    };

}
