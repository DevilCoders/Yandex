#include "plan.h"
#include "chunk.h"

#include <library/cpp/uri/http_url.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/http/io/chunk.h>
#include <util/generic/yexception.h>
#include <util/stream/zlib.h>
#include <util/stream/output.h>
#include <util/stream/null.h>
#include <util/stream/multi.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/string/subst.h>
#include <util/string/vector.h>
#include <util/ysaveload.h>

#include <ctype.h>
#include <util/string/split.h>

namespace {
    THttpHeaders FixHeaders(const THttpHeaders& headers, const THttpURL& url) {
        THttpHeaders h = headers;
        if (!h.HasHeader("Host")) {
            TString hostAndPort = url.PrintS(THttpURL::FlagHostPort);
            h.AddHeader(THttpInputHeader("Host", hostAndPort));
        }
        h.AddHeader(THttpInputHeader("Connection: Close"));

        return h;
    }

    void CheckHttp(const TString& data) {
        THttpOutput out(&Cnull);

        out.Write(data.data(), data.size());
        out.Finish();
    }

    TDevastateRequest ToRequest(const TString& url, const TString& headers) {
        TDevastateRequest request;

        request.Url = url;

        TStringStream stream(headers);
        request.Headers = THttpHeaders(&stream);

        request.Method = "GET";

        return request;
    }

    const TString& GetMethodOrDefault(const TDevastateRequest& request) {
        static const TString Get = "GET";
        static const TString Post = "POST";

        if (request.Method.empty()) {
            return request.Body.empty() ? Get : Post;
        }
        return request.Method;
    }
} // namespace

TDevastateItem::TDevastateItem(const TString& url, const TDuration& toWait, const TString& headers, ui64 planIndex)
    : TDevastateItem(toWait, ToRequest(url, headers), planIndex)
{
}

TDevastateItem::TDevastateItem(const TDuration& toWait, const TString& host, ui16 port, const TString& data, ui64 planIndex)
    : Host_(host)
    , Data_(data)
    , ToWait_(toWait)
    , Port_(port)
    , PlanIndex_(planIndex)
{
    CheckHttp(Data_);
    GenerateUrl();
}


TDevastateItem::TDevastateItem(const TDuration& toWait, const TDevastateRequest& requestDescription, ui64 planIndex)
    : ToWait_(toWait)
    , Port_(80)
    , PlanIndex_(planIndex)
{
    Y_ENSURE(requestDescription.Url.size() < 10 * 1024 * 1024, "Valid url cannot be greater than 10M chars, please check your input. ");
    THttpURL pUrl;

    NUri::TParseFlags parseFlags = THttpURL::FeaturesRecommended | THttpURL::FeatureSchemeFlexible;
    THttpURL::TParsedState parsedState = pUrl.ParseUri(SubstGlobalCopy(requestDescription.Url, "^", "%5E"), parseFlags);
    if (THttpURL::ParsedOK != parsedState) {
        ythrow yexception() << "can not parse url (" << requestDescription.Url.Quote() << ", " << HttpURLParsedStateToString(parsedState) << ")";
    } else if (pUrl.IsNull(THttpURL::FlagHost)) {
        ythrow yexception() << "no host part in url (" << requestDescription.Url.Quote() << ")";
    }

    Host_ = pUrl.GetField(THttpURL::FieldHost);

    const TStringBuf& p = pUrl.GetField(THttpURL::FieldPort);

    if (!p.empty()) {
        Port_ = FromString<ui16>(p);
    }

    TStringStream stream;

    // Starting line
    stream << GetMethodOrDefault(requestDescription) << " ";
    pUrl.Print(stream, THttpURL::FlagPath | THttpURL::FlagQuery);
    stream << " HTTP/1.1\r\n";

    // Headers
    FixHeaders(requestDescription.Headers, pUrl).OutTo(&stream);

    // Body
    stream << "\r\n";
    stream << requestDescription.Body;

    Data_.swap(stream.Str());

    CheckHttp(Data_);
}

TString TDevastateItem::GenerateUrl() const {
    TMemoryInput mi(Data_.data(), Data_.size());
    const TString req = mi.ReadLine();
    TVector<TString> parsed;
    StringSplitter(req).Split(' ').SkipEmpty().Collect(&parsed);

    if (parsed.size() != 3) {
        ythrow yexception() << "incorrect request(" << req.Quote() << ")";
    }

    TString ret = "http://";

    ret += Host_;

    if (Port_ != 80) {
        ret += ":";
        ret += ToString(Port_);
    }

    const TString& query = parsed[1];

    if (!query.empty() && query[0] != '/') {
        ret += '/';
    }

    ret += query;

    return ret;
}

TDevastateItem TDevastateItem::Load(IInputStream* stream, ui64 planIndex) {
    TString data;
    TString host;
    ui16 port = 80;
    TDuration toWait;

    ::Load(stream, host);
    ::Load(stream, data);
    ::Load(stream, toWait);
    ::Load(stream, port);

    return TDevastateItem(toWait, host, port, data, planIndex);
}

void TDevastateItem::Save(IOutputStream* stream) const {
    ::Save(stream, Host_);
    ::Save(stream, Data_);
    ::Save(stream, ToWait_);
    ::Save(stream, Port_);
}

void ForEachPlanItem(IInputStream* input, IDevastatePlanFunctor& func, i64 num, size_t timeLimit) {
    const bool hasDeadline(!!timeLimit);
    const TInstant deadline(Now() + TDuration::Seconds(timeLimit));
    char buf[1];

    ui64 planIndexOffset = 0;
    while (input->Read(buf, 1)) {
        TMemoryInput mi(buf, 1);
        TMultiInput multi(&mi, input);
        TBinaryChunkedInput bi(&multi);
        TZLibDecompress zi(&bi);
        ui64 requestsNumber = 0;

        ::Load(&zi, requestsNumber);

        ui64 n = requestsNumber;
        while (n-- && num-- > 0 && (!hasDeadline || (Now() < deadline))) {
            auto item = TDevastateItem::Load(&zi, planIndexOffset + n);
            const auto verdict = func.Process(item);
            Y_ASSERT(verdict == V_CONTINUE || verdict == V_BREAK);
            if (verdict == V_BREAK) {
                return;
            }
        }

        TransferData(&bi, &Cnull);

        if (!(num > 0 && (!hasDeadline || (Now() < deadline)))) {
            return;
        }

        planIndexOffset += requestsNumber;
    }
}

namespace {
    struct TAdder {
        inline EVerdict operator()(const TDevastateItem& item) {
            Plan->Add(item);
            return V_CONTINUE;
        }

        TDevastatePlan* Plan;
    };
}

TDevastatePlan::TDevastatePlan(IInputStream* stream) {
    TAdder adder = {this};

    ForEachPlanItem(stream, adder);
}

void TDevastatePlan::Save(IOutputStream* output) const {
    TBinaryChunkedOutput bo(output);
    TZLibCompress zos(&bo, ZLib::GZip);

    ::Save(&zos, (ui64)Size());

    for (const_iterator it = Begin(); it != End(); ++it) {
        it->Save(&zos);
    }

    zos.Finish();
    bo.Finish();
}
