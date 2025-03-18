#include "pusher.h"
#include <library/cpp/json/json_reader.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/singleton.h>
#include <util/string/strip.h>

TUnistatPusher::~TUnistatPusher() {
    Stop();
}

void TUnistatPusher::Stop() {
    TGuard<TMutex> g(Mutex);
    if (Thread) {
        Stopped.Signal();
        Thread->Join();
        Thread.Reset();
    }
    HttpClient.Reset();
}

void TUnistatPusher::Start(std::function<void(TUnistat&)> initializer, const TOptions& options /*= TOptions()*/) {
    TGuard<TMutex> g(Mutex);
    if (Thread) {
        ythrow yexception() << "TUnistatPusher already initalized";
    }
    Options = options;
    for (auto& unistat : Unistats) {
        unistat.Reset();
        initializer(unistat);
    }
    TSimpleHttpClient::TOptions httpOptions;
    TDuration socketTimeout = Min(TDuration::MilliSeconds(2500), options.SendPeriod / 2);
    if (socketTimeout == TDuration::Zero()) {
        socketTimeout = TDuration::MilliSeconds(2500);
    }
    HttpClient = MakeHolder<TKeepAliveHttpClient>(options.Host, options.Port, socketTimeout, TDuration::MilliSeconds(100));
    Thread.Reset(SystemThreadFactory()->Run(this).Release());
}

TUnistatPusher& TUnistatPusher::Instance() {
    return *Singleton<TUnistatPusher>();
}

template <>
struct TSingletonTraits<TUnistatPusher> {
    static constexpr size_t Priority = Max<size_t>();
};

void TUnistatPusher::DoExecute() {
    if (Options.SendPeriod == TDuration::Zero()) {
        Stopped.WaitI();
    } else {
        TInstant nextSendTime = Options.SendPeriod.ToDeadLine();
        while (!Stopped.WaitD(nextSendTime)) {
            nextSendTime = Options.SendPeriod.ToDeadLine();
            ui64 currentIndex = AtomicGet(CurrentUnistat);
            AtomicSet(CurrentUnistat, (currentIndex + 1) % UNISTATS_COUNT);
            Sleep(TDuration::MilliSeconds(100));
            TUnistat& unistat = Unistats[currentIndex];
            if (SendToUnistat(unistat)) {
                unistat.ResetSignals();
            }
        }
    }
    for (TUnistat& unistat : Unistats) {
        SendToUnistat(unistat);
    }
}

bool TUnistatPusher::SendToUnistat(TUnistat& unistat) {
    TStringStream reply;
    TKeepAliveHttpClient::THttpCode code;
    try {
        auto ttl = Options.SendPeriod <= TDuration::Seconds(5) ? 0 : Options.SendPeriod.Seconds();
        const TString body = unistat.CreatePushDump(Options.MinPriority, Options.Tags, ttl);
        code = HttpClient->DoPost("/?", body, &reply);
    } catch (THttpRequestException e) {
        ERROR_LOG << "Cannot send unistat to yasm(" << e.GetStatusCode() << "): " << e.what() << Endl;
        return false;
    } catch (yexception e) {
        ERROR_LOG << "Cannot send unistat to yasm: " << e.what() << Endl;
        return false;
    }

    if (code != 200) {
        ERROR_LOG << "Cannot send unistat to yasm(" << code << "): " << reply.Str() << Endl;
        return false;
    }

    NJson::TJsonValue json;
    if (!NJson::ReadJsonTree(&reply, &json)) {
        ERROR_LOG << "Cannot send unistat to yasm: response is not json: " << reply.Str() << Endl;
        return false;
    }
    if (json["status"].GetStringRobust() != "ok") {
        ERROR_LOG << "Cannot send unistat to yasm: status=" << json["status"].GetStringRobust() << "; code=" << json["error_code"].GetStringRobust() << "; error=" << json["error"].GetStringRobust() << Endl;
        return false;
    }
    return true;
}

TUnistatPusher::TOptions::TOptions()
    : Host("localhost")
{}

void TUnistatPusher::TOptions::ParseTags(TStringBuf tags) {
    while (TStringBuf tag = tags.NextTok(';')) {
        TStringBuf name, value;
        tag.TrySplit('=', name, value);
        TString nameStr = Strip(TString(name.data(), name.size()));
        TString valueStr = Strip(TString(value.data(), value.size()));
        if (nameStr) {
            Tags[nameStr] = valueStr;
        }
    }
}
