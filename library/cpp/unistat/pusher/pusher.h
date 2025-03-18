#pragma once

#include <library/cpp/unistat/unistat.h>
#include <library/cpp/http/simple/http_client.h>
#include <util/thread/factory.h>
#include <util/system/event.h>
#include <util/system/mutex.h>

class TUnistatPusher: public IThreadFactory::IThreadAble {
public:
    struct TOptions {
        TOptions();
        void ParseTags(TStringBuf tags);
        TString Host;
        ui16 Port = 11005;
        TDuration SendPeriod = TDuration::Seconds(5);
        NUnistat::TTags Tags;
        int MinPriority = 0;
    };

public:
    ~TUnistatPusher();

    void Stop();
    void Start(std::function<void(TUnistat&)> initializer, const TOptions& options = TOptions());

    static TUnistatPusher& Instance();

    template <typename T>
    Y_FORCE_INLINE bool PushSignalUnsafe(const T& holename, double signal) {
        return Unistats[AtomicGet(CurrentUnistat)].PushSignalUnsafe(holename, signal);
    }

private:
    //IThreadFactory::IThreadAble
    virtual void DoExecute() override;
    bool SendToUnistat(TUnistat& unistat);

private:
    static constexpr ui64 UNISTATS_COUNT = 2;
    TOptions Options;
    THolder<IThreadFactory::IThread> Thread;
    TAutoEvent Stopped;
    TMutex Mutex;
    TUnistat Unistats[UNISTATS_COUNT];
    TAtomic CurrentUnistat;
    THolder<TKeepAliveHttpClient> HttpClient;
};

