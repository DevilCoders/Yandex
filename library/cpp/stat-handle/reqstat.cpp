#include "reqstat.h"

#include <library/cpp/stat-handle/proto/stat.pb.h>

namespace NStat {
    void TRequestStat::RegisterRequest(const TTimeInfo& timing, const TString& clientId) {
        RpsCounter.RegisterRequest();

        if (TimeHist)
            TimeHist->RegisterRequest(timing.Duration);

        TRecord r;
        r.Timing = timing;
        TStat::AppendRecord(clientId, r);
    }

    void TRequestStat::RegisterError(const TStringBuf& type) {
        auto it = Errors.find(type);
        if (it == Errors.end())
            Errors.insert({TString(type), 1});
        else
            it->second += 1;
    }

    void TRequestStat::ToProto(TStatProto& proto) const {
        TStatBase::ToProto(proto);

        proto.SetRps(RpsCounter.RecentRps(TDuration::Minutes(5)));

        for (auto item : Errors) {
            auto* err = proto.AddErrors();
            err->SetType(item.first);
            err->SetCount(item.second);
        }
    }

}
