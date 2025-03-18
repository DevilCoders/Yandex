#pragma once

#include "metriclist.h"
#include <tools/snipmake/snippet_xml_parser/cpp_writer/snippet_dump_xml_writer.h>
#include <kernel/snippets/util/xml.h>
#include <util/stream/buffer.h>
#include <util/stream/printf.h>

namespace NSnippets {

    class ISnippetDumper {
    public:
        virtual void PreDump() {}
        virtual void Dump(const TReqSnip& /*snip*/, const TMetricArray& /*stat*/) {}
        virtual void PostDump() {}
        virtual void Report() {}
        virtual ~ISnippetDumper() {}
    };

    class TSimpleSnippetDumper : public ISnippetDumper {
    private:
        IOutputStream* Out;
        TBufferOutput Line;
    public:
        TSimpleSnippetDumper(IOutputStream* out)
            : Out(out)
        {}

        void Dump(const TReqSnip& snip, const TMetricArray& stat) override {
            Line << snip.Id << "\t" << snip.Query << "\t" << snip.Url << "\t";
            for(EMetricName metric = MN_IS_EMPTY; metric < (stat.Value(MN_IS_EMPTY) ? 1 : MN_COUNT); metric = EMetricName(metric + 1))
                if (stat.Count(metric))
                    Line << metric << ":" << stat.Value(metric) << " ";
            TString res;
            Line.Buffer().AsString(res);
            (*Out) << res << Endl;
        }
    };

    class TSnipStatDumper : public ISnippetDumper {
    private:
        IOutputStream* Out;
    public:
         TSnipStatDumper(IOutputStream* out)
            : Out(out)
        {}

        void Dump(const TReqSnip& snip, const TMetricArray& stat) override {
            (*Out) << snip << "Metrics: "<<Endl;
            for(EMetricName metric = MN_IS_EMPTY; metric < (stat.Value(MN_IS_EMPTY) ? 1 : MN_COUNT); metric = EMetricName(metric + 1))
                if (stat.Count(metric))
                    Printf(*Out, "%34s\t  %f\n", ToString(metric).c_str(), stat.Value(metric));
            (*Out) << "=====================================================" << Endl;
        }
    };

    class TXmlSnippetDumper : public ISnippetDumper {
    private:
        TSnippetDumpXmlWriter Writer;
    public:
        TXmlSnippetDumper(IOutputStream* out)
            : Writer(*out)
        {}

        void PreDump() override {
            Writer.Start();
            Writer.StartPool("q");
        }

        void Dump(const TReqSnip& snip, const TMetricArray& stat) override {
            TString tmpUrl = WideToUTF8(snip.Url);
            if (!(tmpUrl.StartsWith("http://") || tmpUrl.StartsWith("https://"))) {
                tmpUrl = "http://" + tmpUrl;
            }

            Writer.StartQDPair(snip.Query, tmpUrl, "1", "213", snip.B64QTree);
            TReqSnip snippet = snip;

            for(EMetricName metric = MN_IS_EMPTY; metric < (stat.Value(MN_IS_EMPTY) ? 1 : MN_COUNT); metric = EMetricName(metric + 1))
                if (stat.Count(metric))
                    snippet.FeatureString += ToString(metric) + ":" + ToString(stat.Value(metric)) + " ";

            Writer.WriteSnippet(snippet);
            Writer.FinishQDPair();
        }

        void PostDump() override {
            Writer.FinishPool();
            Writer.Finish();
        }
    };
}
