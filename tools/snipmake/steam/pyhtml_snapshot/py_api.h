#pragma once

/*
 * A sligtly retarded SWIG-compatible wrapper around the libhtml_snapshot
 */

#include <tools/snipmake/steam/html_snapshot/misc.h>
#include <tools/snipmake/steam/html_snapshot/zora.h>
#include <tools/snipmake/steam/html_snapshot/direct.h>
#include <tools/snipmake/steam/html_snapshot/rewritehtml.h>

#include <dict/recognize/docrec/recognizer.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NSteam {


class TAsyncFetchResult;

class TAsyncFetchResultPy
{
private:
    TSimpleSharedPtr<TAsyncFetchResult> Queue;

public:
    TAsyncFetchResultPy();
    ~TAsyncFetchResultPy();

    // TODO: return TSimpleSharedPtr<>
    TFetchedDoc* WaitForCompletion(unsigned int timeout_ms);
    TSimpleSharedPtr<TAsyncFetchResult> GetQueue()
    {
        return Queue;
    }
};

class TZoraFetcherPy
{
    TZoraFetcher Fetcher;
    TDuration Timeout;

public:
    TZoraFetcherPy(const TString& sourceName, bool userproxy, bool ipv4, int timeoutMsec);
    void Submit(const TString& url, const TString& jobId, TAsyncFetchResultPy* receiver);
    void Terminate();
};

class TDirectFetcherPy
{
    TDirectFetcher Fetcher;
    TDuration Timeout;

public:
    TDirectFetcherPy(const TString& userAgent, int timeoutMsec);
    void Submit(const TString& url, const TString& jobId, TAsyncFetchResultPy* receiver);
    void Terminate();
};

class TContentLink
{
public:
    TString Url;
    TString ContentTypeHint;

    TContentLink()
    {

    }

    TContentLink(const TString& url, const TString& contentTypeHint)
        : Url(url)
        , ContentTypeHint(contentTypeHint)
    {
    }
};

class TContentParseResult
{
public:
    TString RewrittenContent;
    ECharset Encoding;
    bool Error;
    TString ErrorMessage;

    TContentParseResult()
        : Encoding(CODES_UNKNOWN)
        , Error(false)
    {
    }
};

/* Opaque wrapper for parsed URLs. Can be passed to Python callbacks */
class IUrlWrapper
{
public:
    virtual TString MergeWithBase(const TString& relUrl) const = 0;
    virtual ~IUrlWrapper() {}
};

/* Wrapper for Python callable with signature (str) */
class IUrlXform
{
public:
    // TODO: multiple args
    virtual TString Call(const TStringBuf& url) = 0;

    virtual ~IUrlXform()
    {
    }
};

/* Wrapper for Python callable with signature (str, object, int, int) */
class ICssXform
{
public:
    virtual TString Call(const TStringBuf& css, const IUrlWrapper& baseUrl, ECharset& encoding, bool isInline) = 0;

    virtual ~ICssXform()
    {
    }
};

class TWebContentParser
{
private:
    THtmlRewriter Rewriter;
    TRecognizer Recognizer;

public:
    TWebContentParser(const TString& configDir);
    ~TWebContentParser();

    // TODO: return TSimpleSharedPtr<>
    TContentParseResult* RewriteContent(const TString& content, const TString& url, const TString& mimeType, ECharset encoding, IUrlXform& urlMapFn, ICssXform& cssFn);
};

const char* GetEncodingName(ECharset encoding);

}
