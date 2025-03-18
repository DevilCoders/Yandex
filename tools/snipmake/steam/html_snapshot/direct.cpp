#include "direct.h"

#include <yweb/webutil/url_fetcher_lib/fetcher.h>
#include <yweb/webutil/url_fetcher_lib/http_headers.h>

#include <util/generic/algorithm.h>
#include <util/generic/list.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <library/cpp/threading/future/legacy_future.h>
#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <util/string/strip.h>

namespace NSteam
{

void ParseContentType(const THeadersMap& headers, TString& contentType, TString& charset) {
    charset.clear();
    contentType.clear();

    THeadersMap::const_iterator hdr = headers.find("content-type");
    if (hdr == headers.end()) {
        return;
    }

    TStringBuf headerBuf(hdr->second);
    if (!headerBuf) {
        return;
    }
    TStringBuf token = headerBuf.NextTok(';');
    token = StripString(token);
    if (token.empty())
        return;

    contentType = token;

    while (headerBuf.NextTok(';', token)) {
        TString key(token), value;
        TStringBuf nameBuf;
        TStringBuf valueBuf(key);
        nameBuf.NextTok('=', nameBuf);
        nameBuf = StripString(nameBuf);
        valueBuf = StripString(valueBuf);
        if (value.length() >= 2 && value[0] == '\"' && value.back() == '\"') {
            value = value.substr(1, value.length() - 2);
            Strip(value, value);
        }

        if (stricmp(key.c_str(), "charset") == 0) {
            charset = value;
        }
    }
}


void TDirectFetcher::Start(TDirectFetcher* host)
{
    TFetchParams fp;
    fp.CONNECT_TIMEOUT = host->Timeout;
    fp.READ_WRITE_TIMEOUT = host->Timeout;
    fp.NThreads = 8;
    if (!!host->UserAgent) {
        fp.USER_AGENT = host->UserAgent; //"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.9; rv:28.0) Gecko/20100101 Firefox/28.0";
    }
    fp.SaveContent = true;
    ::Fetch(host, host, fp);
}

TDirectFetcher::TDirectFetcher(TDuration timeout, const TString& userAgent)
    : Timeout(timeout)
    , Eof(false)
{
    Cerr << "Please do not use the direct fetcher on dev servers" << Endl;

    UserAgent = Strip(userAgent);
    if (!!UserAgent) {
        Cerr << "Custom user agent: \"" << UserAgent << "\"" << Endl;
    }

    Runner.Reset(new NThreading::TLegacyFuture<>(std::bind(&Start, this)));
}

bool TDirectFetcher::IsEof() const
{
    return Eof;
}

void TDirectFetcher::Terminate()
{
    Eof = true;
}

bool TDirectFetcher::HasNext()
{
    TGuard inLock(InGuard);
    const TInstant now = Now();
    TTaskList::iterator task = InTasks.begin();
    while (task != InTasks.end()) {
        if (task->Deadline < now) {
            Fail(*task, "Timed out waiting to send request", HTTP_PROXY_REQUEST_TIME_OUT);
            task = InTasks.erase(task);
        }
        break;
    }

    return task != InTasks.end();
}

const TCheckUrl* TDirectFetcher::Next()
{
    TGuard inLock(InGuard);
    TTaskList::iterator task = InTasks.begin();
    if (task == InTasks.end()) {
        return nullptr;
    }

    FillCheckUrl(task->Url, CheckUrl);
    Cerr << GetName() << "(" << Now() << "): start download \"" << CheckUrl.Url << Endl;
    SetInFlight(*task);
    InTasks.erase(task);
    return &CheckUrl;
}

void TDirectFetcher::OnFetchResult(const ::TFetchResult& res, const TCheckUrl& chUrl, IInputStream* content)
{
    TFetchedDoc doc;
    doc.HttpCode = res.Code;
    doc.Url = chUrl.Url;

    if (res.Code != RESULT_OK) {
        doc.Failed = true;
        Complete(doc);
        return;
    }

    doc.Failed = false;
    doc.Url = chUrl.Url;
    doc.FinalUrl = res.Location;
    doc.Language = LANG_UNK;
    doc.Encoding = CODES_UNKNOWN;
    doc.MimeType = MIME_UNKNOWN;
    doc.RawMimeType.clear();

    if (content) {
        THeadersMap headers;
        FetchHeaders(content, headers);
        doc.Content = content->ReadAll();
        TString charsetName;
        ParseContentType(headers, doc.RawMimeType, charsetName);
        if (!!charsetName) {
            doc.Encoding = CharsetByName(charsetName);
        }
        if (!!doc.RawMimeType) {
            doc.MimeType = mimeByStr(doc.RawMimeType.data());
        }
    }
    Complete(doc);
}


}
