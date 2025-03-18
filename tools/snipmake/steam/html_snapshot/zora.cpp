#include "zora.h"

#include <library/cpp/http/fetch/exthttpcodes.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/algorithm.h>
#include <util/generic/noncopyable.h>
#include <util/generic/singleton.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/codepage.h>
#include <util/string/strip.h>

namespace NSteam
{
    using namespace NZoraClient;

    TZoraFetcher::TZoraFetcher(TDuration timeout, const TString& source, bool userproxy, bool ipv4)
        : Source(source)
        , Timeout(timeout)
        , ReqType(userproxy ? NFetcherMsg::USERPROXY : NFetcherMsg::ONLINE)
    {
        //FIXME: Move syslog initialization somewhere closer to the user
        SysLogInstance().ResetBackend(THolder(new TSysLogBackend(GetName(), TSysLogBackend::TSYSLOG_LOCAL1, TSysLogBackend::LogPerror)));
        NZoraClient::TConfig zcfg;
        zcfg.NeedToWriteLogel = true;
        zcfg.MessageBusNumWorkers = 10;
        //zcfg.MessageBusSendTimeout = 100*200;
        //zcfg.MessageBusTotalTimeout = 40*1000;
        zcfg.NeedToWriteDocument = true;
        zcfg.HeaderInDocument = true;
        zcfg.WriteExtInformation = true;
        zcfg.MaxRedirectsLevel = 5;
        zcfg.RedirectOutputMode = 1;
        zcfg.IsDaemon = true;
        zcfg.Source = Source;
        zcfg.MaxRejects = 1;
        zcfg.SecondQuota = 50;
        zcfg.MaxInFlight = 200000;
        if (ipv4) {
            zcfg.IpType = NZoraClient::TConfig::Ipv4;
        }

        TimeoutSeconds = int(Timeout.SecondsFloat() + 0.5);
        if (!TimeoutSeconds) {
            ++TimeoutSeconds;
        }

        ZoraCl.Reset(new TZoraClient(zcfg, this, this));
        Runner.Reset(new NThreading::TLegacyFuture<>(std::bind(&TZoraClient::Start, ZoraCl.Get())));
    }

    TBusFetcherRequest* TZoraFetcher::GetNextRequest()
    {
        TGuard inLock(InGuard);
        const TInstant now = Now();
        TTaskList::iterator task = InTasks.begin();

        while (task != InTasks.end()) {
            if (task->Deadline < now) {
                Fail(*task, "Timed out waiting to send request", HTTP_PROXY_REQUEST_TIME_OUT);
                task = InTasks.erase(task);
            }
            else if (InFlight.find(task->Url) != InFlight.end()) {
                SetInFlight(*task);
                task = InTasks.erase(task);
            }
            break;
        }

        if (task == InTasks.end()) {
            return nullptr;
        }

        THolder<TBusFetcherRequest> request(new TZoraClientRequest());
        request->Record.set_clienttype(Source);
        request->Record.set_timeout(TimeoutSeconds);
        request->Record.set_priority(1);
        request->Record.set_freshness(24*60*60);
        request->Record.mutable_context()->set_logelversion(1); // only logel version 1 supported
        request->Record.mutable_context()->set_requesttype(ReqType);
        request->Record.set_url(task->Url);
        SetInFlight(*task);
        InTasks.erase(task);
        return request.Release();
    }

    static TString GetFirstUrl(const TString& urls) {
        size_t t = urls.find(' ');
        TString res(urls.data(), urls.data() + (t == TString::npos ? urls.size() : t));
        return res;
    }

    static TString GetLastUrl(const TString& urls) {
        size_t t = urls.rfind(' ');
        TString res(urls.data() + (t == TString::npos ? 0 : t), urls.data() + urls.size());
        return res;
    }

    static TString ExtractContentType(const THttpHeaders& headers)
    {
        TString contentTypeHeader;
        for(THttpHeaders::TConstIterator it = headers.Begin(); it != headers.End(); ++it) {
            TCiString name(it->Name());
            if (name == "Content-Type") {
                contentTypeHeader = it->Value();
                contentTypeHeader.to_lower();
                break;
            }
        }
        return contentTypeHeader;
    }

    void TZoraFetcher::OutputLogel(const TBusFetcherRequest *, const TLogel<TUnknownRec> *logel)
    {
        if (logel->RecSig() != TUpdUrlLRec::RecordSig) {
            return;
        }

        TString url = GetFirstUrl(NLogUtil::Url(logel));
        const int httpCode = NLogUtil::HttpCode(logel);
        TString info = NLogUtil::LogelInfo(logel);

        TFetchedDoc doc;
        doc.Url = url;
        doc.Failed = true;
        doc.HttpCode = httpCode;
        doc.ErrorMessage = info;
        Complete(doc);
    }

    void TZoraFetcher::OutputDocument(const TBusFetcherRequest *, const char *contentAndHeaders, size_t len, const char *urlAndInfo)
    {
        TMemoryInput inputStream(contentAndHeaders, len);
        THttpInput httpStream(&inputStream);
        const THttpHeaders& headers = httpStream.Headers();
        TString contentType = ExtractContentType(headers);
        TFetchedDoc doc;

        doc.RawMimeType = TString{StripString(TStringBuf(contentType).Before(';'))};
        doc.Content = httpStream.ReadAll();

        TVector<TStringBuf> v;

        TStringBuf s = urlAndInfo;
        while (!!s) {
            TStringBuf keyval;
            s.Split('\t', keyval, s);
            if (keyval.StartsWith("http")) {
                v.push_back(keyval);
                continue;
            }
            TStringBuf key;
            TStringBuf value;
            keyval.Split('=', key, value);
            if (key == "encoding") {
                doc.Encoding = CharsetByName(TString{value});
                if (!doc.RawEncoding) {
                    doc.RawEncoding = TString{value};
                }
            }
            else if (key == "lang") {
                doc.Language = LanguageByName(TString{value});
            }
            else if (key == "mime") {
                if (!doc.RawMimeType) {
                    doc.RawMimeType = TString{value};
                }
                doc.MimeType = mimeByStr(doc.RawMimeType.data());
            }
        }

        if (v.empty()) {
            return;
        }

        doc.Url = GetFirstUrl(TString(v[0].data(), v[0].size()));
        doc.FinalUrl = GetLastUrl(TString(v[0].data(), v[0].size()));

        StripInPlace(doc.FinalUrl);
        StripInPlace(doc.Url);
        Complete(doc);
    }

    void TZoraFetcher::OnBadResponse(TBusFetcherRequest *request, TBusFetcherResponse *response)
    {
        TFetchedDoc doc;
        const TString url = TZoraClientRequest::CalcUrl(*request);
        doc.Url = url;
        doc.Failed = true;
        doc.HttpCode = response->Record.GetHttpCode();
        if (doc.HttpCode) {
            doc.ErrorMessage = ExtHttpCodeStr(doc.HttpCode);
        }
        else {
            doc.ErrorMessage = "Unexplained Zora error (ILogger::OnBadResponse)";
        }
        Complete(doc);
    }

    void TZoraFetcher::Terminate()
    {
        ZoraCl->Terminate(true);
    }
}
