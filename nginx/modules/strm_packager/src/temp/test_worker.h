#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

#include <util/generic/map.h>

namespace NStrm::NPackager::NTemp {
    class TTestWorker: public TRequestWorker {
    public:
        TTestWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

        ~TTestWorker();

    private:
        class TTestSubrequest: public ISubrequestWorker {
        public:
            TTestSubrequest(TTestWorker& owner);
            void AcceptHeaders(const THeadersOut& headers) override;
            void AcceptData(char const* const begin, char const* const end) override;
            void SubrequestFinished(const TFinishStatus status) override;

        private:
            TTestWorker& Owner;
        };

        void Work() override;

        TString id;
        int count;
        bool binary;

        TMap<size_t, TString> SubData;
    };

}
