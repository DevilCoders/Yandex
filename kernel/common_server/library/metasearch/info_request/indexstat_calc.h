#pragma once

#include <search/meta/info.h>

namespace NRTYServer {

    class TMetaInfoIndexStatCalculator : public IMetaInfoCalculator {
    public:
        void Execute(const TMetaSearch& owner, TMetaSearchContext& ctx, const TCgiParameters& cgiParam, const TRequestParams &params, IOutputStream& infoBuf) override;

    private:
        class TMetaInfoTask : public IMetaInfoTask {
        public:
            using TKeyPrefixStat = TMap<i64, i64>;
        public:
            TMetaInfoTask(TClientInfo* ci, ui64& sentCount, ui64& wordCount, TKeyPrefixStat& kps);
            void FillRequestData(NScatter::TRequestData& rd) const override;
            NScatter::TParseResult DoParse(const NScatter::TTaskReply& reply) override;

        private:
            ui64& SentCount;
            ui64& WordCount;
            TKeyPrefixStat& Kps;
        };

        static TFactory::TRegistrator<TMetaInfoIndexStatCalculator> Registrator;
    };

}
