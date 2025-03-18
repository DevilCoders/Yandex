#pragma once

#include <tools/segutils/segcommon/data_utils.h>
#include <tools/segutils/segcommon/qutils.h>

#include <kernel/segnumerator/segnumerator.h>
#include <kernel/segutils/numerator_utils.h>
#include <kernel/tarc/disk/unpacker.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/lcs/lcs_via_lis.h>
#include <library/cpp/numerator/numerate.h>

#include <util/charset/wide.h>
#include <util/digest/murmur.h>
#include <util/generic/hash.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/zlib.h>
#include <util/string/util.h>
#include <utility>

namespace NSegutils {

using namespace NLCS;

struct TOpts {
    enum EMode {
        M_NONE, M_LOG, M_ARC, M_JSON
    };

    THolder<TParserContext> Ctx;

    THtmlFileReader LearnReader;
    THtmlFileReader TestReader;

    TString RefDataPath;
    TString LogFileDirectoryPath;

    ui32 MinWords;

    bool SkipSport;
    bool Headers;
    bool LearnMode;

    EMode Mode;

    bool RefHasHeaders;

    TOpts()
        : MinWords()
        , SkipSport()
        , Headers()
        , LearnMode()
        , Mode(M_NONE)
        , RefHasHeaders(true)
    {}

    bool Process(int, const Opt&) { return false; }
    TString OptsString() const { return ""; }
    void CallString() { }
    void UsageDescription() { }

    void UsageAndExit(const char* me, int code);
    EMode ProcessOpts(int argc, const char**argv);

    bool IsArcMode() const {
        return M_ARC == Mode;
    }

    bool IsLogMode() const {
        return M_LOG == Mode;
    }

    bool IsJsonMode() const {
        return M_JSON == Mode;
    }

    const THtmlFileReader& GetReader() const {
        return LearnMode ? LearnReader : TestReader;
    }
};


struct TDocProcessor {
    typedef NSegm::TSegmentatorHandler<> TSegHandler;

    const TOpts* Opts;

    THtmlFile Data;
    TSegHandler Handler;

    TString Url;
    NSegm::THashValues Title;
    NSegm::THashValues Text;

    NSegm::THashValues RefHeader;
    NSegm::THashValues RefContent;

    NSegm::THashValues RealHeader;
    NSegm::THashValues RealContent;

    TDocProcessor(const TOpts& opts, const TString& url, const TMaybe<THtmlFile>& doc = TMaybe<THtmlFile>())
        : Opts(&opts)
        , Url(url)
    {
        if (!doc) {
            Data = opts.GetReader().Read(opts.GetReader().Url2File.find(url)->second);
        } else {
            Data = *doc;
        }
        Y_VERIFY(Data.Url == url, " ");
    }

    virtual ~TDocProcessor() {
    }

    virtual bool ProcessLive();
    virtual bool ProcessRef(const TUtf16String& title, const TUtf16String& text);

    virtual bool Postprocess();

    void GetTokensFrom(NSegm::THashValues& hh, TPosting b, TPosting e) {
        Handler.GetTokens().CollectHashes(hh, NSegm::TSpan(b, e));
    }
};

struct TArcProcessor {
    const TOpts* Opts;

    TArcProcessor(const TOpts& opts)
        : Opts(&opts)
    {}

    virtual ~TArcProcessor() {
    }

    virtual bool OnDoc(const TArchiveHeader*, const TArchiveIterator&, const TString& url) = 0;

    bool OnDocBase(TDocProcessor&, const TArchiveHeader*, const TArchiveIterator&);

    virtual void Process();
};

struct TLogDoc {
    TString Url;
    TUtf16String Header;
    TUtf16String Content;

    TLogDoc();
    TLogDoc(const TString& line);
    void AppendContent(const TUtf16String& content);
};

struct TLogProcessor {
    const TOpts* Opts;

    TLogProcessor(const TOpts& opts)
        : Opts(&opts)
    {}

    virtual ~TLogProcessor() {
    }

    virtual bool OnDoc(const TLogDoc&) = 0;

    bool OnDocBase(TDocProcessor& proc, const TLogDoc& doc) {
        return proc.ProcessRef(doc.Header, doc.Content) && proc.ProcessLive();
    }

    virtual void Process();
};

template <typename TMarkup, typename TMarkupDoc>
struct TNewsJsonProcessor {
    const TOpts* Opts;
    size_t DocCount;

    TNewsJsonProcessor(const TOpts &opts)
        : Opts(&opts)
        , DocCount()
    {}

    virtual ~TNewsJsonProcessor() {
    }

    virtual bool OnDoc(const TMarkupDoc& doc) = 0;

    bool OnDocBase(TDocProcessor& proc, const TMarkupDoc& doc) const {
        return proc.ProcessRef(UTF8ToWide(doc.Title), UTF8ToWide(doc.Text)) && proc.ProcessLive();
    }

    virtual void Process() {
        TMarkup markup(Opts->RefDataPath);
        TVector<TMarkupDoc> records = markup.GetRecordsVector();
        DocCount = records.size();
        for (size_t i = 0; i < records.size(); ++i) {
            OnDoc(records[i]);
        }
    }
};

}
