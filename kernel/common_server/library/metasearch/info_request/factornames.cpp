#include <search/meta/context.h>
#include <search/meta/doc.h>
#include <search/meta/info.h>
#include <search/meta/metasearch.h>

namespace {
    class TMetaFactorNames : public IMetaInfoCalculator {
    private:
        struct TFactor {
            TVector<TString> Sources;
            TString Name;
            ui32 Index;
        };
        using TFactors = TMultiMap<ui32, TFactor>;

        class TTask : public IMetaInfoTask {
        public:
            using IMetaInfoTask::IMetaInfoTask;

            inline const TFactors& GetFactors() const {
                return Factors;
            }

            virtual void FillRequestData(NScatter::TRequestData& rd) const override {
                rd.Url = TString("info=factornames");
            }
            virtual NScatter::TParseResult DoParse(const NScatter::TTaskReply& reply) override {
                TStringInput input(reply.Data);
                TString line;
                while (input.ReadLine(line)) {
                    auto splitted = SplitString(line, "\t");
                    if (splitted.size() < 2) {
                        continue;
                    }

                    TFactor factor;
                    factor.Index = FromString<ui32>(splitted[0]);
                    factor.Name = splitted[1];
                    if (splitted.size() > 2) {
                        factor.Sources = SplitString(splitted[2], ",");
                    }

                    Factors.insert(std::make_pair(factor.Index, factor));
                }
                return { Finished };
            }

        private:
            TFactors Factors;
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
            ctx.RunTasks("factornames", tasks_);

            TFactors factors;
            for (auto&& task : tasks) {
                const TString& source = task->Source()->SearchSource()->Descr + "@" + task->Source()->SearchSource()->Group;
                for (auto f : task->GetFactors()) {
                    auto range = factors.equal_range(f.first);
                    auto p = range.first;
                    for (; p != range.second; ++p) {
                        if (f.second.Name == p->second.Name)
                            break;
                    }

                    f.second.Sources.push_back(source);
                    if (p != range.second) {
                        p->second.Sources.insert(p->second.Sources.end(), f.second.Sources.begin(), f.second.Sources.end());
                    } else {
                        factors.insert(f);
                    }
                }
            }
            for (auto&& factor : factors) {
                const TFactor& f = factor.second;
                infoBuf << f.Index << '\t' << f.Name;
                if (f.Sources.size() != owner.ClientCount()) {
                    infoBuf << '\t' << JoinStrings(f.Sources.begin(), f.Sources.end(), ",");
                }
                infoBuf << Endl;
            }
        }
    };
}

IMetaInfoCalculator::TFactory::TRegistrator<TMetaFactorNames> MetaFactorNamesRegistrator("factornames");
