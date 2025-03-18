#include "test_worker.h"

#include <nginx/modules/strm_packager/src/base/config.h>

namespace NStrm::NPackager::NTemp {
    TTestWorker::TTestWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_test:")
    {
        Y_ENSURE(Config.TestURI.Defined());

#if 0
        TVector<int, TNgxAllocator<int>> v(4, GetAllocator());
        TVector<int, TNgxAllocator<int>> v2(4, GetAllocator());

        for (size_t i = 0; i < v.size(); ++i) {
            v2[i] = i;
        }

        void* pv2 = (void*)&v2[0];
        Cout << "v2 " << pv2 << Endl;

        std::swap(v, v2);

        void* pv = (void*)&v[0];
        Cout << "v " << pv << Endl;

        for (int a : v) {
            Cout << "bjj " << a << " " << Endl;
        }

        Y_ENSURE(pv == pv2);
#endif
    }

    TTestWorker::~TTestWorker() {
        Clog << (TStringBuilder() << "== desctructor id : " << id << " count: " << count << "  pid: " << ngx_pid << "\n");
    }

    // static
    void TTestWorker::CheckConfig(const TLocationConfig& config) {
        Y_ENSURE(config.TestURI.Defined());
    }

    void TTestWorker::Work() {
        const TStringBuf uriSuffix = GetArg("urisuf", false).GetOrElse("");
        count = GetArg<i64>("count", false).GetOrElse(Config.TestSubrequestsCount.GetOrElse(1));
        binary = GetArg<bool>("binary", false).GetOrElse(false);
        id = GetArg("id", false).GetOrElse("-");

        Clog << (TStringBuilder() << "== work id : " << id << " count: " << count << "  pid: " << ngx_pid << "\n");

        TTestSubrequest* subrequest = GetPoolUtil<TTestSubrequest>().New(*this);

        for (int i = 0; i < count; ++i) {
            CreateSubrequest(
                {
                    .Uri = *Config.TestURI + uriSuffix,
                    .RangeBegin = GetArg<ui64>("begin", false),
                    .RangeEnd = GetArg<ui64>("end", false),
                    .Background = true,
                },
                subrequest);

            if (!binary) {
                Cout << "TTestWorker::Work start " << i << Endl;
            }
        }
    }

    TTestWorker::TTestSubrequest::TTestSubrequest(TTestWorker& owner)
        : Owner(owner)
    {
    }

    void TTestWorker::TTestSubrequest::AcceptHeaders(const THeadersOut& headers) {
        Owner.GetLogger(TLOG_INFO).Stream() << "TTestWorker::AcceptHeaders: status: " << headers.Status();
        Y_ENSURE(headers.Status() == NGX_HTTP_OK || headers.Status() == NGX_HTTP_PARTIAL_CONTENT);
    }

    void TTestWorker::TTestSubrequest::AcceptData(char const* const begin, char const* const end) {
        Y_ENSURE(begin && end > begin);

        if (!Owner.binary) {
            Cout << "\n [[";
            for (char const* c = begin; c != end; ++c) {
                Cout << *c;
                Owner.SubData[(size_t)this] += *c;
            }
            Cout << "]]" << Endl;
        }

        TString original(begin, end - begin);

        if (Owner.binary) {
            Owner.SendData({Owner.HangDataInPool(original), true});
        } else {
            Owner.SendData({Owner.HangDataInPool("[" + original + "]"), true});
            Owner.SendData({Owner.HangDataInPool("{" + original + "}"), false});
        }
    }

    void TTestWorker::TTestSubrequest::SubrequestFinished(const TFinishStatus status) {
        CheckFinished(status);

        --Owner.count;

        if (!Owner.binary) {
            Cout << "code: " << status.Code << " tsr " << (void*)this << " full data: [[" << Owner.SubData[(size_t)this] << "]]" << Endl;
            Cout << "TTestWorker::SubrequestFinished count: " << Owner.count << Endl;
        }

        if (Owner.count == 0) {
            Owner.Finish();
        }
    }

}
