#pragma once

#include "snipinfos.h"
#include "metriclist.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <util/generic/ptr.h>
#include <util/generic/utility.h>

namespace NSnippets {

    struct TSnipMetricsPreloaded {
        TString Description; // description of the metrics calculator. For example, for which type of snippets it is used.
        TWordFilter SWF;
        TPornoTermWeight PornoWeighter;
        IOutputStream* Out;

        TSnipMetricsPreloaded(const TString& desc, const TString& stopwordsFile, const TString& pornoWordsConfig, IOutputStream* out)
            : Description(desc)
            , SWF()
            , Out(out)
        {
            PornoWeighter.Init(pornoWordsConfig.data());
            if (!SWF.InitStopWordsList(stopwordsFile.data()))
                ythrow yexception() << "unable to load stopwords.lst from " << stopwordsFile.data();
        }

    private:
        TSnipMetricsPreloaded();
        TSnipMetricsPreloaded& operator=(const TSnipMetricsPreloaded& rv);
    };

    class TSnipMetricsCalculator {
    private:
        THolder<TSnipMetricsPreloaded>  LocalPreloadedHolder;
        const TSnipMetricsPreloaded& LocalPreloaded;

        THolder<NSnippets::TConfig> Cfg;
        const TPornoTermWeight& PornoWeighter;

    protected:
        IOutputStream* Out;
        const TString& Description; // description of the metrics calculator. For example, for which type of snippets it is used.

    private: // Private methods
        virtual void CalculateSerpMetrics(const TQueryy& query, const TReqSerp& reqSerp);
        virtual void CalculateMetrics(const TQueryy& query, const TReqSnip& reqSnip);

        // called to clear Cfg.Alloc
        void ResetCfg() {
            NSnippets::TConfigParams cfgp;
            cfgp.AppendExps.push_back("syn_count");
            cfgp.AppendExps.push_back("ignore_dup_ext");
            cfgp.StopWords = &LocalPreloaded.SWF;
            Cfg.Reset(new NSnippets::TConfig(cfgp));
        }

    public:
        TSnipMetricsCalculator(const TString& desc, const TString& stopwordsFile, const TString& pornoWordsConfig, IOutputStream* out)
            : LocalPreloadedHolder(new TSnipMetricsPreloaded(desc, stopwordsFile, pornoWordsConfig, out))
            , LocalPreloaded(*LocalPreloadedHolder)
            , PornoWeighter(LocalPreloaded.PornoWeighter)
            , Out(LocalPreloaded.Out)
            , Description(LocalPreloaded.Description)
        {
            ResetCfg();
        }

        TSnipMetricsCalculator(const TSnipMetricsPreloaded& preloaded)
            : LocalPreloaded(preloaded)
            , PornoWeighter(preloaded.PornoWeighter)
            , Out(preloaded.Out)
            , Description(preloaded.Description)
        {
            ResetCfg();
        }

        virtual void operator() (const TReqSerp& reqSerp) {
            if (reqSerp.RichRequestTree.Get() == nullptr)
                return;

            ResetCfg();
            // use default memory allocator in case we are using preloaded config (usually in a multithreaded environment)
            const TQueryy query(reqSerp.RichRequestTree->Root.Get(), *Cfg);

            this->CalculateSerpMetrics(query, reqSerp);
            for (const auto& reqSnip : reqSerp.Snippets) {
                this->PreCalculateMetrics(query, reqSnip); // Call some initialization and so on.
                this->CalculateMetrics(query, reqSnip);
                this->PostCalculateMetrics(query, reqSnip);
            }
        }

        virtual void Report() = 0;
        virtual ~TSnipMetricsCalculator() { }
    protected:
        virtual void PreCalculateMetrics(const TQueryy& /*query*/, const TReqSnip& /*reqSnip*/) { }
        virtual void PostCalculateMetrics(const TQueryy& /*query*/, const TReqSnip& /*reqSnip*/) { }
        virtual void ProcessMetricValue(EMetricName /*metricName*/, double /*value*/) = 0;
    };

    class TSnipMetricsAveCalculator : public TSnipMetricsCalculator {
    private:
        enum EMaxQuery {
            MQ_WORDS = 5
        };
        TVector<TMetricArray> Stat;
        int QueryWords;
    public:
        TSnipMetricsAveCalculator(const TString& desc, const TString& stopwordsFile, const TString& pornoWordsConfig, IOutputStream* out)
            : TSnipMetricsCalculator(desc, stopwordsFile, pornoWordsConfig, out)
            , Stat(MQ_WORDS + 1)
            , QueryWords(0)
        {
        }

        void Report() override {
            (*Out) << Description << Endl;
            for (int words = 0; words < MQ_WORDS; ++words) {
                const TMetricArray& stat = Stat[words];
                (*Out) << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\nQuery length:" << words << Endl;
                (*Out) << "Snippets count: " << stat.Count(MN_IS_EMPTY) << Endl;
                for(EMetricName metric = MN_IS_EMPTY; metric < MN_COUNT; metric = EMetricName(metric + 1))
                    if (stat.Count(metric))
                        (*Out) << metric << ',' << stat.Value(metric) / stat.Count(metric) << Endl;
            }
        }

    protected:
        void PreCalculateMetrics(const TQueryy& query, const TReqSnip& /*reqSnip*/) override {
            QueryWords = Min<int>(query.UserPosCount(), MQ_WORDS);
        }

        void ProcessMetricValue(EMetricName metricName, double value) override {
            Stat[QueryWords].SumValue(metricName, value);
            Stat[0].SumValue(metricName, value); // обобщенные метрики
        }
    };


    template <class TSnipDumper>
    class TSnipMetricsDumper : public TSnipMetricsCalculator {
    private:
        TMetricArray Stat;
        TSnipDumper Dumper;
    public:
        TSnipMetricsDumper(const TString& desc, const TString& stopwordsFile, const TString& pornoWordsConfig, IOutputStream* out)
            : TSnipMetricsCalculator(desc, stopwordsFile, pornoWordsConfig, out)
            , Dumper(out)
        {
            Dumper.PreDump();
        }

        void Report() override {
        }

    ~TSnipMetricsDumper() override
    {
        Dumper.PostDump();
    }

    protected:
        void ProcessMetricValue(EMetricName metricName, double value) override {
            Stat.SetValue(metricName, value);
        }
        void PostCalculateMetrics(const TQueryy& /*query*/, const TReqSnip& reqSnip) override {
            Dumper.Dump(reqSnip, Stat);
            Stat.Reset();
        }
    };
}
