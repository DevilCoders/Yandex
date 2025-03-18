#include "py_api.h"
#include <tools/snipmake/steam/html_snapshot/rewritecss.h>
#include <tools/snipmake/steam/html_snapshot/linkmap.h>
#include <library/cpp/charset/codepage.h>
#include <Python.h>

namespace NSteam {

TAsyncFetchResultPy::TAsyncFetchResultPy()
    : Queue(new TAsyncFetchResult())
{
}

TAsyncFetchResultPy::~TAsyncFetchResultPy()
{
}

struct TPythonAllowThreads
{
    PyThreadState* State;

    TPythonAllowThreads()
    {
        State = PyEval_SaveThread();
    }

    ~TPythonAllowThreads()
    {
        PyEval_RestoreThread(State);
    }
};

TFetchedDoc* TAsyncFetchResultPy::WaitForCompletion(unsigned int timeout_ms)
{
    std::pair<bool, TSimpleSharedPtr<TFetchedDoc> > result;
    {
        TPythonAllowThreads noGIL;
        result = Queue->Next(TDuration::MilliSeconds(timeout_ms));
    }
    if (!result.first) {
        return nullptr;
    }
    return new TFetchedDoc(*result.second);
}

static const int HARD_TIMEOUT_MARGIN_MS = 2000;


TZoraFetcherPy::TZoraFetcherPy(const TString& sourceName, bool userproxy, bool ipv4, int timeoutMsec)
    : Fetcher(TDuration::MilliSeconds(timeoutMsec), sourceName, userproxy, ipv4)
    , Timeout(TDuration::MilliSeconds(timeoutMsec + HARD_TIMEOUT_MARGIN_MS))
{
}

void TZoraFetcherPy::Submit(const TString& url, const TString& jobId, TAsyncFetchResultPy* receiver)
{
    Fetcher.Add(TFetchTask(url, jobId, Timeout.ToDeadLine(), receiver->GetQueue()));
}

void TZoraFetcherPy::Terminate()
{
    Fetcher.Terminate();
}

TDirectFetcherPy::TDirectFetcherPy(const TString& userAgent, int timeoutMsec)
    : Fetcher(TDuration::MilliSeconds(timeoutMsec), userAgent)
    , Timeout(TDuration::MilliSeconds(timeoutMsec + HARD_TIMEOUT_MARGIN_MS))
{
}

void TDirectFetcherPy::Submit(const TString& url, const TString& jobId, TAsyncFetchResultPy* receiver)
{
    Fetcher.Add(TFetchTask(url, jobId, Timeout.ToDeadLine(), receiver->GetQueue()));
}

void TDirectFetcherPy::Terminate()
{
    Fetcher.Terminate();
}

class TUrlWrapper: public IUrlWrapper
{
private:
    THttpURL Url;
    ECharset Encoding;

public:
    TUrlWrapper(const THttpURL& url, ECharset enc);
    TUrlWrapper(const TString& url, ECharset enc);

    TString MergeWithBase(const TString& relUrl) const override;
};

class TUrlMapAdapter: public IUrlMapper
{
private:
    IUrlXform& Mapper;

public:
    TUrlMapAdapter(IUrlXform& mapper)
        : Mapper(mapper)
    {
    }

    TString RewriteUrl(const TString& url, ELinkType linkType) override
    {
        TString mimeHint;
        if (linkType == LINK_CSS) {
            mimeHint = "css";
        }
        else if (linkType == LINK_FRAME) {
            mimeHint = "html";
        }
        else if (linkType == LINK_IMAGE) {
            mimeHint = "image";
        }
        //TODO proper multiple arguments
        //FIXME ugly hack
        return Mapper.Call(url + "\t" + mimeHint);
    }
};

class TCssXformAdapter: public ICssConverter
{
public:
    ICssXform& CssFn;

    TCssXformAdapter(ICssXform& cssFn)
        : CssFn(cssFn)
    {
    }

    TString ConvertCss(const TString& css, const THttpURL& baseUrl, ECharset encoding, bool isInlineStyle) override
    {
        TUrlWrapper urlWrapper(baseUrl, encoding);
        return CssFn.Call(css, urlWrapper, encoding, isInlineStyle);
    }
};

TWebContentParser::TWebContentParser(const TString& configDir)
    : Rewriter(configDir)
    , Recognizer(configDir + LOCSLASH_S + "dict.dict")
{
}

TWebContentParser::~TWebContentParser()
{

}

TContentParseResult* TWebContentParser::RewriteContent(
        const TString& content,
        const TString& url,
        const TString& mimeType,
        ECharset encoding,
        IUrlXform& urlMapFn,
        ICssXform& cssFn)
{
    THolder<TContentParseResult> result(new TContentParseResult());
    result->Encoding = encoding;
    TUrlMapAdapter mapper(urlMapFn);
    TCssXformAdapter cssConverter(cssFn);
    TConvertResult convResult;

    if (mimeType == "text/html") {
        TPreparsedHtml preparsed(content, url);
        if (encoding == CODES_UNKNOWN || encoding == CODES_UNSUPPORTED) {
            preparsed.Recognize(Recognizer);
            result->Encoding = preparsed.GetCharset();
        }
        else {
            preparsed.SetCharset(encoding);
        }
        if (!preparsed.IsValid()) {
            return nullptr;
        }
        convResult = Rewriter.TryConvert(preparsed, mapper, cssConverter);
    }
    else if (mimeType == "text/css") {
        TUrlWrapper parsedUrl(url, encoding);
        convResult.Data = cssFn.Call(content, parsedUrl, encoding, false);
        convResult.Done = true; // TODO: categorize and handle errors
    }

    if (convResult.Done) {
        result->RewrittenContent = convResult.Data;
    }
    else {
        result->Error = true;
        result->ErrorMessage = convResult.ErrorMessage;
    }
    return result.Release();
}

const char* GetEncodingName(ECharset encoding)
{
    return NameByCharset(encoding);
}


TUrlWrapper::TUrlWrapper(const THttpURL& url, ECharset enc)
    : Url(url)
    , Encoding(enc)
{
}

TUrlWrapper::TUrlWrapper(const TString& url, ECharset enc)
    : Encoding(enc)
{
    auto parseResult = Url.ParseAbsUri(url, NUri::TFeature::FeaturesRecommended, 0, NUri::TScheme::SchemeHTTP, enc);
    if (parseResult != NUri::TState::ParsedOK) {
        Url.Parse("error://", NUri::TFeature::FeaturesBare);
    }
}

TString TUrlWrapper::MergeWithBase(const TString& relUrl) const
{
    THttpURL result;
    if (!MergeUrlWithBase(relUrl, Url, result, Encoding)) {
        return TString("error:") + relUrl;
    } else {
        return result.PrintS();
    }
}

}
