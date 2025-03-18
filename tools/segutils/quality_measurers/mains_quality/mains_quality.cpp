#include <tools/segutils/quality_measurers/segqualitycommon/qualitycommon.h>
#include <yweb/news/tools/pr_meter_lib/markup.h>

using namespace NSegm;
using namespace NLCS;

namespace NSegutils {

struct TResult {
    TString Url;
    TString File;
    TStat Header;
    TStat Content;
    TStat BinaryContent;

    TResult(const TString& url = TString(), const TString& file = TString())
        : Url(url)
        , File(file)
        , Header()
        , Content()
        , BinaryContent()
    {
    }

    void Print(const TString& id) const {
        printf("%s\t%s\t%s\t%.1f\t%.1f\t%.3f\t%.1f\t%.1f\t%.3f\n",
               id.data(), Url.data(), File.data(),
               Content.Pr() * 100, Content.Re() * 100, Content.F1(),
               Header.Pr() * 100,  Header.Re() * 100,  Header.F1());
    }
};

typedef TVector<TResult> TResults;

typedef TOpts TMainsOpts;

struct TMainsDocProcessor : public TDocProcessor {
    THashValues Header;
    THashValues Content;

    TMainsDocProcessor(const TOpts& opts, const TString& url, const TMaybe<THtmlFile>& doc = TMaybe<THtmlFile>())
        : TDocProcessor(opts, url, doc)
    {}

    bool ProcessLive() override {
        bool res = TDocProcessor::ProcessLive();
        if (Opts->RefHasHeaders) {
            const TMainHeaderSpans& hs = Handler.GetMainHeaderSpans();
            for (TMainHeaderSpans::const_iterator it = hs.begin(); it != hs.end(); ++it)
                GetTokensFrom(Header, it->Begin, it->End);

            const TMainContentSpans& cs = Handler.GetMainContentSpans();
            for (TMainContentSpans::const_iterator it = cs.begin(); it != cs.end(); ++it)
                GetTokensFrom(Content, it->Begin, it->End);
        } else {
            TSpans s;

            const TMainContentSpans& cs = Handler.GetMainContentSpans();
            s.insert(s.end(), cs.begin(), cs.end());

            const TMainHeaderSpans& hs = Handler.GetMainHeaderSpans();
            s.insert(s.end(), hs.begin(), hs.end());

            StableSort(s.begin(), s.end());

            for (TSpans::iterator it = s.begin(); it != s.end(); ++it) {
                if (it != s.begin() && (it - 1)->Contains(it->Begin)) {
                    it = s.erase(it);
                    --it;
                }
            }

            for (TSpans::const_iterator it = s.begin(); it != s.end(); ++it) {
                GetTokensFrom(Content, it->Begin, it->End);
            }
        }

        return res;
    }
};


struct TMainsCommonProcessor {
    TResults Results;
    TStat BinaryContent;

    bool PostDoc(TMainsDocProcessor& proc, const TString& id) {
        TResult res(proc.Data.Url, proc.Data.FileName);

        if (!proc.Postprocess())
            return false;

        res.Header.AddTp(MeasureLCS<size_t>(proc.Header, proc.RealHeader));
        res.Header.AddFp(proc.Header.size() - res.Header.Tp());
        res.Header.AddFn(proc.RealHeader.size() - res.Header.Tp());
        if (proc.Header.empty() && !proc.RealHeader.empty()) {
            res.Header.SetPrUndef();
        }

        res.Content.AddTp(MeasureLCS<size_t>(proc.Content, proc.RealContent));
        res.Content.AddFp(proc.Content.size() - res.Content.Tp());
        res.Content.AddFn(proc.RealContent.size() - res.Content.Tp());
        if (proc.Content.empty() && !proc.RealContent.empty()) {
            res.Content.SetPrUndef();
        }

        BinaryContent.AddTp((proc.Content.size() > 0 && proc.RefContent.size() > 0) ||
                            (proc.Content.size() == 0 && proc.RefContent.size() == 0));
        BinaryContent.AddFp(proc.Content.size() > 0 && proc.RefContent.size() == 0);
        BinaryContent.AddFn(proc.Content.size() == 0 && proc.RefContent.size() > 0);

        Results.push_back(res);
        res.Print(id);
        return true;
    }

    void ProcessResults() {
        if (Results.empty())
            return;
        float cpr = 0;
        size_t cprc = 0;
        float cre = 0;

        float hpr = 0;
        size_t hprc = 0;
        float hre = 0;

        for (TResults::const_iterator it = Results.begin(); it != Results.end(); ++it) {
            if (!it->Content.PrUndef()) {
                cpr += it->Content.Pr();
                ++cprc;
            }
            cre += it->Content.Re();
            if (!it->Header.PrUndef()) {
                hpr += it->Header.Pr();
                ++hprc;
            }
            hre += it->Header.Re();
        }

        Cout << "cpr:  " << cpr / cprc << Endl;
        Cout << "cre:  " << cre / Results.size() << Endl;

        cpr /= cprc;
        cre /= Results.size();

        Cout << "cf1:  " << 2* cpr * cre / (cpr + cre) << Endl;

        Cout << "hpr:  " << hpr / hprc << Endl;
        Cout << "hre:  " << hre / Results.size() << Endl;

        hpr /= hprc;
        hre /= Results.size();

        Cout << "hf1:  " << 2* hpr * hre / (hpr + hre) << Endl;

        Cout << "Binary maincontent precision: " << BinaryContent.Pr() << Endl;
        Cout << "Binary maincontent recall: " << BinaryContent.Re() << Endl;
        Cout << "Total " << BinaryContent.Total() << Endl;
    }
};

struct TMainsArcProcessor : public TArcProcessor, public TMainsCommonProcessor {
    TMainsArcProcessor(const TMainsOpts& opts)
        : TArcProcessor(opts)
    {}

    bool OnDoc(const TArchiveHeader* h, const TArchiveIterator& it, const TString& url) override {
        TMainsDocProcessor proc(*Opts, url);

        if (!OnDocBase(proc, h, it))
            return false;

        return PostDoc(proc, ToString(h->DocId));
    }

    void Process() override {
        TArcProcessor::Process();
        ProcessResults();
    }
};

struct TMainsLogProcessor : public TLogProcessor, public TMainsCommonProcessor {
    TMainsLogProcessor(const TMainsOpts& opts)
        : TLogProcessor(opts)
    {}

    bool OnDoc(const TLogDoc& doc) override {
        TMainsDocProcessor proc(*Opts, doc.Url);
        if (!OnDocBase(proc, doc))
            return false;

        return PostDoc(proc, "");
    }

    void Process() override {
        TLogProcessor::Process();
        ProcessResults();
    }
};

struct TMainsNewsJsonProcessor : public TNewsJsonProcessor<TPrMarkup, TPrMarkupRecord>, public TMainsCommonProcessor {
    typedef TNewsJsonProcessor<TPrMarkup, TPrMarkupRecord> TBase;
    TMainsNewsJsonProcessor(const TMainsOpts& opts)
        : TBase(opts)
    {}

    bool OnDoc(const TPrMarkupRecord& doc) override {
        THtmlFile htmlDoc;
        htmlDoc.Url = doc.Url;
        htmlDoc.Html = doc.HtmlContent;
        TMainsDocProcessor proc(*Opts, doc.Url, htmlDoc);
        if (!OnDocBase(proc, doc))
            return false;

        return PostDoc(proc, "");
    }

    void Process() override {
        TBase::Process();
        ProcessResults();
    }
};
}

int main(int argc, const char**argv) {
    using namespace NSegutils;
    TMainsOpts opts;
    opts.ProcessOpts(argc, argv);

    if (opts.IsArcMode()) {
        TMainsArcProcessor proc(opts);
        proc.Process();
    } else if (opts.IsLogMode()){
        TMainsLogProcessor proc(opts);
        proc.Process();
    } else if (opts.IsJsonMode()) {
        TMainsNewsJsonProcessor proc(opts);
        proc.Process();
    }
}
