#include <tools/segutils/quality_measurers/segqualitycommon/qualitycommon.h>
#include <yweb/news/tools/pr_meter_lib/markup.h>

#include <kernel/segmentator/main_header_impl.h>
#include <kernel/segmentator/main_content_impl.h>

namespace NSegutils {
using namespace NSegm;
using namespace NSegm::NPrivate;

struct TFeaturesProcessor : public TDocProcessor {
    TMemoryPool Pool;
    TAutoPtr<TFeatureArrayVector> Features;
    TString Url;

    TFeaturesProcessor(const TOpts& opts, const TString& url, const TMaybe<THtmlFile>& doc = TMaybe<THtmlFile>())
        : TDocProcessor(opts, url, doc)
        , Pool(1024)
        , Features(opts.Headers ? (TFeatureArrayVector*)new THeaderFeatures(10000)
                                : (TFeatureArrayVector*)new TSegmentFeatures(10000, &Pool))
        , Url(url)
    {
        if (opts.Headers)
            Handler.SetSkipMainHeader();
        else
            Handler.SetSkipMainContent();
    }

    bool Postprocess() override {
        if (!TDocProcessor::Postprocess())
            return false;

        const TDocContext& ctx = Handler.GetDocContext();
        const THeaderSpans& hp = Handler.GetHeaderSpans();
        const TSegmentSpans& sp = Handler.GetSegmentSpans();
        Features->Calculate(ctx, hp, sp);

        for (TFeatureArrayVector::iterator it = Features->begin(); it != Features->end(); ++it) {
            THashValues h;
            GetTokensFrom(h, sp[it->Number].Begin, sp[it->Number].End);

            if (h.empty())
                continue;

            it->Url = Url;
            it->Lcs = NLCS::MeasureLCS<size_t>(h, RealContent);
            it->RealContentSize = RealContent.size();
            it->ContentSize = h.size();
            float recallWithout = float(it->RealContentSize - it->Lcs) / it->RealContentSize;
            float precisionWith = float(it->RealContentSize) / (it->RealContentSize + it->ContentSize - it->Lcs);
            float f1With = precisionWith / (1.0 + precisionWith);
            float f1Without = recallWithout / (1.0 + recallWithout);
            it->Relev = f1With - f1Without;
            it->Weight = 1.0;
        }

        return true;
    }
};

struct TFeaturesArcProcessor : public TArcProcessor {
    TFeaturesArcProcessor(const TOpts& opts)
        : TArcProcessor(opts)
    {}

    bool OnDoc(const TArchiveHeader* h, const TArchiveIterator& i, const TString& url) override {
        TFeaturesProcessor proc(*Opts, url);
        bool res = OnDocBase(proc, h, i);
        if (!res || !proc.Postprocess())
            return false;

        proc.Features->Print(Cout);

        return true;
    }
};

struct TFeaturesLogProcessor : public TLogProcessor {
    TFeaturesLogProcessor(const TOpts& opts)
        : TLogProcessor(opts)
    {}

    bool OnDoc(const TLogDoc& doc) override
    {
        TFeaturesProcessor proc(*Opts, doc.Url);
        bool res = OnDocBase(proc, doc);

        if (!res || !proc.Postprocess())
            return false;

        proc.Features->Print(Cout);
        return true;
    }
};

struct TFeaturesJsonProcessor : public TNewsJsonProcessor<TPrMarkup, TPrMarkupRecord> {
    TFeaturesJsonProcessor(const TOpts& opts)
        : TNewsJsonProcessor(opts)
    {}

    bool OnDoc(const TPrMarkupRecord& doc) override
    {
        THtmlFile htmlDoc;
        htmlDoc.Url = doc.Url;
        htmlDoc.Html = doc.HtmlContent;
        TFeaturesProcessor proc(*Opts, doc.Url, htmlDoc);
        bool res = OnDocBase(proc, doc);

        if (!res || !proc.Postprocess())
            return false;

        {
            static TMutex outputLock;
            TGuard<TMutex> guard(outputLock);
            proc.Features->Print(Cout);
        }

        return true;
    }
};

}

int main(int argc, const char** argv) {
    using namespace NSegutils;
    TOpts opts;
    opts.ProcessOpts(argc, argv);

    if (opts.IsArcMode()) {
        TFeaturesArcProcessor proc(opts);
        proc.Process();
    }

    if (opts.IsLogMode()) {
        TFeaturesLogProcessor proc(opts);
        proc.Process();
    }

    if (opts.IsJsonMode()) {
        TFeaturesJsonProcessor proc(opts);
        proc.Process();
    }
}
