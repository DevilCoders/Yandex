#include <search/meta/context.h>
#include <search/meta/doc.h>
#include <search/meta/info.h>
#include <search/meta/metasearch.h>

#include <library/cpp/histogram/rt/histogram.h>

#include <library/cpp/json/json_value.h>

namespace {
    class TMetaHistogramCalculator : public IMetaInfoCalculator {
    private:
        class TTask : public IMetaInfoTask {
        public:
            using IMetaInfoTask::IMetaInfoTask;

            const NRTYServer::THistograms& GetHistograms() const {
                return Histograms;
            }

            virtual void FillRequestData(NScatter::TRequestData& rd) const override {
                rd.Url = TString("info=histogram");
            }
            virtual NScatter::TParseResult DoParse(const NScatter::TTaskReply& reply) override {
                NJson::TJsonValue json;
                NJson::ReadJsonFastTree(reply.Data, &json, true);
                Histograms.Deserialize(json);
                return { Finished };
            }
        private:
            NRTYServer::THistograms Histograms;
        };

    public:
        virtual void Execute(
            const TMetaSearch& owner,
            TMetaSearchContext& ctx,
            const TCgiParameters& cgiParam,
            const TRequestParams& params,
            IOutputStream& infoBuf
        ) override {
            Y_UNUSED(cgiParam);
            Y_UNUSED(params);

            TVector<TIntrusivePtr<TTask>> tasks;
            for (size_t i = 0; i < owner.ClientCount(); ++i) {
                TClientInfo &ci = ctx.ClientsInfo[i];
                tasks.push_back(new TTask(&ci));
            }

            NScatter::TTaskList tasks_ = { tasks.begin(), tasks.end() };
            ctx.RunTasks("histogram", tasks_);

            NRTYServer::THistograms histograms;
            for (auto&& task : tasks) {
                histograms.Merge(task->GetHistograms());
            }
            infoBuf << histograms.Serialize<NJson::TJsonValue>().GetStringRobust();
        }
    };
}

IMetaInfoCalculator::TFactory::TRegistrator<TMetaHistogramCalculator> MetaHistogramCalculatorRegistrator("histogram");
