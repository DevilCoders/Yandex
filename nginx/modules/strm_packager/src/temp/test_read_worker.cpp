#include "test_read_worker.h"

#include <nginx/modules/strm_packager/src/base/config.h>

namespace NStrm::NPackager::NTemp {
    TTestReadWorker::TTestReadWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_test_read_worker:")
    {
    }

    void TTestReadWorker::CheckConfig(const TLocationConfig& /*config*/) {
    }

    void TTestReadWorker::Work() {
        const int poolsCount = 10;
        const int srCount = 100;
        // const int srCount = 1;

        // const ui64 begin = 21391;
        // const ui64 end = 88428;
        // const ui64 end = 1024 * 1024 * 1.8;
        const ui64 end = 65604629376;

#if 0
        ui8* const buf = GetPoolUtil<ui8>().Alloc(end - begin);
        TVector<NThreading::TFuture<TSimpleBlob>> futures;
        for (int i = 0; i < srCount; ++i) {
            futures.push_back(CreateSubrequest(
                TSubrequestParams {
                    .Uri = "/slow/" + ToString(i % poolsCount) + "/audio_4_822ba9b8db9d8820099c10de5aa938b6.mp4",
                    // .Uri = "/slow/" + ToString(i % poolsCount) + "/sss" + ToString(i) + "/audio_4_822ba9b8db9d8820099c10de5aa938b6.mp4",
                    .RangeBegin = begin,
                    .RangeEnd = end,
                    .Background = false,
                },
                206,
                buf,
                buf + (end - begin),
                true));
        }
#else
        // test timers!
        ui64 bsize = 30 * 1024 * 1024;
        ui8* const buf = GetPoolUtil<ui8>().Alloc(bsize);
        TVector<NThreading::TFuture<void>> futures;
        for (int i = 0; i < srCount; ++i) {
            TNgxTimer* timer = MakeTimer();

            NThreading::TPromise<void> promise = NThreading::NewPromise();

            futures.push_back(promise);

            timer->ResetCallback(MakeIndependentCallback([this, promise, i, buf, bsize]() {
                ui64 ibegin = ngx_random() % 10000;
                ibegin = (end - bsize) * ibegin / 10000;

                CreateSubrequest(
                    TSubrequestParams{
                        // .Uri = "/slow/" + ToString(i % poolsCount) + "/audio_4_822ba9b8db9d8820099c10de5aa938b6.mp4",
                        .Uri = "/slow/" + ToString(i % poolsCount) + "/bjj2",
                        .RangeBegin = ibegin,
                        .RangeEnd = ibegin + bsize,
                        .Background = false,
                    },
                    206,
                    buf,
                    buf + bsize,
                    true)
                    .Subscribe(MakeFatalOnException([promise](const NThreading::TFuture<TSimpleBlob>& f) mutable {
                        f.GetValue();
                        promise.SetValue();
                    }));
            }));

            timer->ResetTime(100);
        }

#endif
        PackagerWaitExceptionOrAll(futures).Subscribe(MakeFatalOnException([this](const NThreading::TFuture<void>& future) {
            future.GetValue();

            SendData({HangDataInPool(TString("done!\n")), true});
            Finish();
        }));
    }

}
