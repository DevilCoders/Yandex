#pragma once

#include "replier.h"
#include "exception.h"
#include <search/output_context/stream_output_context.h>
#include <kernel/daemon/base_http_client.h>
#include <search/common/profile.h>

namespace NProfile {
    class TMeasurement;
}

template <class TRequestData>
class IHttpClient: public TCommonHttpClient<TRequestData> {
private:
    TInstant RequestStartTime = Now();
public:
    TInstant GetRequestStartTime() const {
        return RequestStartTime;
    }
    virtual const TBlob& GetBuf() const = 0;
};

template <class TContext>
class THttpClientImpl: public IHttpClient<typename TContext::TRequestData> {
private:
    using TBase = IHttpClient<typename TContext::TRequestData>;
    using TBase::ReleaseConnection;
    using TBase::ProcessHeaders;
    using TBase::RD;
    using TBase::Buf;
protected:
    using TBase::HttpServ;
    using TBase::ProcessSpecialRequest;
    using TBase::Output;
public:
    virtual ~THttpClientImpl() {
        try {
            ReleaseConnection();
        } catch (...) {
            DEBUG_LOG << "Connection releasing failed: " << CurrentExceptionMessage() << Endl;
            OnReleaseConnectionFailure();
        }
    }

    virtual bool Reply(void* /*ThreadSpecificResource*/) override {
        THolder<NProfile::TMeasurement> measurementHolder;
        measurementHolder.Reset(StartProfile());

        Output().Flush();
        if (!ProcessHeaders()) {
            return true;
        }

        const auto& serverOptions = HttpServ()->Options();
        RD.Scan();
        RD.SetHost(serverOptions.Host, serverOptions.Port);

        auto context = MakeAtomicShared<TContext>(this);
        try {
            OnBeginProcess(context);

            if (ProcessSpecialRequest() || !ProcessAuth()) {
                return false;
            }

            IHttpReplier::TPtr searchReplier = DoSelectHandler(context);
            CHECK_WITH_LOG(!!searchReplier);
            searchReplier.Release()->Reply();
        } catch (const TSearchException& e) {
            MakeErrorPage(context, e.GetHttpCode(), e.what());
        } catch (...) {
            MakeErrorPage(context, HTTP_INTERNAL_SERVER_ERROR, CurrentExceptionMessage());
        }
        return false;
    }

    virtual void OnReleaseConnectionFailure() {
    }

    virtual void OnBeginProcess(IReplyContext::TPtr /*context*/) {
    }

    virtual bool ProcessAuth() {
        return true;
    }

    const TBlob& GetBuf() const override {
        return Buf;
    }

    TSearchRequestData& MutableRequestData() {
        return RD;
    }

protected:
    virtual void MakeErrorPage(IReplyContext::TPtr context, ui32 code, const TString& error) const {
        ::MakeErrorPage(context, code, error);
    }

private:
    virtual IHttpReplier::TPtr DoSelectHandler(IReplyContext::TPtr context) = 0;
    // Lifted verbatim from search/daemons/httpsearch/yshttp.cpp(YSClientRequest::StartProfile())

    NProfile::TMeasurement* StartProfile() {
        NProfile::TMeasurement* result = nullptr;
        if (FirstRun) { // Yes, this is very much a hack, but there's no other reliable way for the
            NProfile::Reset(); // first of the (possibly many) calls to Reply() to start from clean slate.
            result = new NProfile::TMeasurement("requestTotal");
        }
        FirstRun = false; // Save for hacking THttpServer itself, which we're currently avoiding.
        return result;
    }

private:
    bool FirstRun = true;
    THolder<NSearch::TStreamOutputProxyContext> OutContext_;
};

template <class TRequestDataParam>
class TCommonHttpReplyContext: public IReplyContext {
public:
    using TRequestData = TRequestDataParam;
    using IHttpClient = ::IHttpClient<TRequestData>;
protected:
    THolder<IHttpClient> Client;
    virtual const TBlob& DoGetBuf() const override {
        return Client->GetBuf();
    }

private:
    NSearch::TStreamOutputContext OutContext;
    TMap<TString, TString> HttpHeaders;
    bool ConnectionReleased = false;

public:
    virtual const TBaseServerRequestData& GetBaseRequestData() const override {
        return Client->GetBaseRequestData();
    }

    virtual TBaseServerRequestData& MutableBaseRequestData() override {
        return Client->MutableBaseRequestData();
    }

    TCommonHttpReplyContext(IHttpClient* client)
        : Client(client)
        , OutContext(Client->Output())
    {
    }

    NSearch::IOutputContext& Output() override {
        CHECK_WITH_LOG(!ConnectionReleased);
        return OutContext;
    }

    virtual long GetRequestedPage() const override {
        return 0;
    }

    virtual bool IsHttp() const override {
        return true;
    }

    virtual bool IsLocal() const override {
        return Client->IsLocal();
    }

    virtual TInstant GetRequestStartTime() const override {
        return Client->GetRequestStartTime();
    }

    virtual void DoMakeSimpleReply(const TBuffer& buf, int code = HTTP_OK) override {
        MakeSimpleReplyImpl(buf, code);
    }

    template <class T>
    void MakeSimpleReplyImpl(const T& buf, int code = HTTP_OK) noexcept {
        MakeSimpleReplyImpl<T>(Output(), buf, code, HttpHeaders);
        ReleaseConnection();
    }

    void ReleaseConnection() noexcept try {
        ConnectionReleased = true;
        Client->ReleaseConnection();
    } catch (...) {
        ERROR_LOG << "Exception during ReleaseConnection: " << CurrentExceptionMessage() << Endl;
    }

    template <class T>
    static void MakeSimpleReplyImpl(IOutputStream& output, const T& buf, int code = HTTP_OK, const TMap<TString, TString>& headers = TMap<TString, TString>()) noexcept try {
        NSearch::TStreamOutputContext ctx(output);
        MakeSimpleReplyImpl(ctx, buf, code, headers);
    } catch (...) {
        ERROR_LOG << "Exception during MakeSimpleReply: " << CurrentExceptionMessage() << Endl;
    }

    template <class T>
    static void MakeSimpleReplyImpl(NSearch::IOutputContext& output, const T& buf, int code = HTTP_OK, const TMap<TString, TString>& headers = TMap<TString, TString>()) noexcept try {
        TStringBuf CrLf = "\r\n";
        output.Write(TStringBuf("HTTP/1.1 "));
        output.Write(HttpCodeStrEx(code));
        output.Write(CrLf);
        for (auto&& header : headers) {
            output.Write(header.first);
            output.Write(TStringBuf(": "));
            output.Write(header.second);
            output.Write(CrLf);
        }

        output.Write(TStringBuf("Content-Length: "));
        output.Write(ToString(buf.size()));
        output.Write(CrLf);
        output.Write(CrLf);
        output.Write(TStringBuf(buf.data(), buf.size()));
        output.Flush();
    } catch (...) {
        ERROR_LOG << "Exception during MakeSimpleReply: " << CurrentExceptionMessage() << Endl;
    }

    virtual void AddReplyInfo(const TString& key, const TString& value, const bool rewrite) override {
        const TString lKey = ToLowerUTF8(key);
        if (rewrite) {
            HttpHeaders[lKey] = value;
        } else {
            HttpHeaders.emplace(lKey, value);
        }
    }

    const TMap<TString, TString>& GetReplyHeaders() const override {
        return HttpHeaders;
    }

    virtual void Print(const TStringBuf& data) override {
        Output().Write(data);
    }
};
