#include "test_cache_worker.h"

#include <nginx/modules/strm_packager/src/base/config.h>

namespace NStrm::NPackager::NTemp {
    TTestCacheWorker::TTestCacheWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_test_cache:")
        , Cache(config.TestShmCacheZone->Data())
    {
    }

    // static
    void TTestCacheWorker::CheckConfig(const TLocationConfig& config) {
        Y_ENSURE(config.TestShmCacheZone.Defined());
    }

    void TTestCacheWorker::Work() {
        const size_t count = *GetArg<size_t>("count", true);
        const size_t size = *GetArg<size_t>("size", true);

        i64* minTimeDiff = GetPoolUtil<i64>().New();
        i64* maxTimeDiff = GetPoolUtil<i64>().New();
        *minTimeDiff = 1e9;
        *maxTimeDiff = -1e9;
        size_t* counterEq = GetPoolUtil<size_t>().New();
        size_t* counterNe = GetPoolUtil<size_t>().New();
        *counterEq = 0;
        *counterNe = 0;

        {
            TStringBuilder resp;
            resp << " this: " << (void*)this << " pid: " << getpid() << " \n";
            SendData({HangDataInPool<TString>(resp), true});
        }

        for (size_t i = 0; i < count; ++i) {
            TStringBuilder dataPart;
            dataPart << "#" << i << "#";

            TString data;
            while (data.Size() < size) {
                data += dataPart;
            }

            const auto getter = [ this, data ]() -> auto{
                // Clog << " sr create: this: " << (void*)this << " pid: " << getpid() << " \n";
                const auto f = CreateSubrequest(
                    TSubrequestParams{
                        .Uri = "/cache_test_server_proxy",
                        .Args = data,
                    },
                    NGX_HTTP_OK);
                // f.Subscribe(MakeFatalOnException([this](const NThreading::TFuture<TBuffer>&){
                //     Clog << " sr done: this: " << (void*)this << " pid: " << getpid() << " \n";
                // }));
                return f;
            };

            // const auto subff = getter();  // without cache
            const auto subff = Cache.Get(*this, data, getter); // with cache

            subff.Subscribe(MakeFatalOnException([count, counterEq, counterNe, this, data, minTimeDiff, maxTimeDiff](const NThreading::TFuture<TBuffer>& future) {
                // Clog << " subff done: this: " << (void*)this << " pid: " << getpid() << " \n";
                const TBuffer& fdata = future.GetValue();
                const size_t fdataSize = fdata.Size();
                char const* const fdataPtr = fdata.Data();
                TString timeStr;
                size_t i = 0;
                for (; i < fdataSize && fdataPtr[i] != '/'; ++i) {
                    timeStr += fdataPtr[i];
                }
                const ui64 fdataTime = FromString<ui64>(timeStr);
                const ui64 curTime = TInstant::Now().MilliSeconds();
                bool equal = true;
                for (int k = 0; k < 1000 && equal; ++k) {
                    if (i >= fdataSize || fdataPtr[i] != '/') {
                        equal = false;
                        break;
                    }
                    ++i;
                    for (size_t l = 0; l < data.Size(); ++l, ++i) {
                        if (i >= fdataSize || fdataPtr[i] != data[l]) {
                            equal = false;
                            break;
                        }
                    }
                }

                ++*(equal ? counterEq : counterNe);

                const i64 timeDiff = curTime - fdataTime;
                *minTimeDiff = Min(*minTimeDiff, timeDiff);
                *maxTimeDiff = Max(*maxTimeDiff, timeDiff);

                if (equal) {
                    // Clog << "  time diff: " << timeDiff << Endl;
                } else {
                    GetLogger(TLOG_ERR).Stream()
                        << "\n  equal " << equal
                        << "\n  curTime " << curTime
                        << "\n  fdataTime " << fdataTime
                        << "\n  time diff: " << timeDiff
                        << "\n";
                }

                if (*counterEq + *counterNe == count) {
                    TStringBuilder resp;
                    resp
                        << " eq: " << *counterEq << " ne: " << *counterNe
                        << " minTimeDiff: " << *minTimeDiff << " maxTimeDiff: " << *maxTimeDiff
                        << " this: " << (void*)this
                        << " \n";
                    SendData({HangDataInPool<TString>(resp), false});
                    Finish();
                }
            }));
        }
    }

}
