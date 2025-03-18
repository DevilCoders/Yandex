#include <library/cpp/dolbilo/stat.h>
#include <library/cpp/getopt/opt2.h>
#include <library/cpp/uri/http_url.h>

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/printf.h>

class TDumpStat {
public:
    struct TUrlInfo {
        TString Url;
        ui64 Time = 0;

        TUrlInfo() {
        }

        TUrlInfo(const TString& url, ui64 time)
            : Url(url)
            , Time(time)
        {
        }

        bool operator<(const TUrlInfo& info) const {
            return Url < info.Url;
        }
    };
    typedef TVector<TUrlInfo> TUrlInfos;

    TUrlInfos Aggregated;

    TDumpStat(const TString& filename) {
        TFileInput fIn(filename);

        TUrlInfos infos;

        try {
            while (true) {
                TDevastateStatItem item(&fIn);
                infos.push_back(TUrlInfo(item.Url(), (item.EndTime() - item.StartTime()).MicroSeconds()));
            }
        } catch (const TDevastateStatItem::TCanNotLoad&) {
        }

        Sort(infos.begin(), infos.end());

        TString prevUrl = "";
        ui64 prevCount = 0;
        ui64 prevTime = 0;

        for (TUrlInfos::const_iterator toUrl = infos.begin(); toUrl != infos.end(); ++toUrl) {
            if (toUrl->Url != prevUrl) {
                if (prevCount)
                    Aggregated.push_back(TUrlInfo(prevUrl, prevTime / prevCount));
                prevUrl = toUrl->Url;
                prevCount = 1;
                prevTime = toUrl->Time;
            } else {
                ++prevCount;
                prevTime += toUrl->Time;
            }
        }
        if (prevCount)
            Aggregated.push_back(TUrlInfo(prevUrl, prevTime / prevCount));
    }
};

static bool CmpByDiff(const TDumpStat::TUrlInfo& lhs, const TDumpStat::TUrlInfo& rhs) {
    return lhs.Time < rhs.Time;
}

static TString UrlDescr(const TString& url) {
    THttpURL httpUrl;
    if (THttpURL::ParsedOK != httpUrl.ParseUri(url, THttpURL::FeaturesDefault, ui32(1 << 16)))
        Cerr << "!!! bad url '" << url << "'" << Endl;
    TString cgi = httpUrl.Get(THttpURL::FieldQuery);
    TCgiParameters cgiParams(cgi.data());
    return Sprintf("%s\t[%s]", url.data(), cgiParams.Get("user_request").data());
}

void Diff(const TDumpStat& oldStat, const TDumpStat& newStat) {
    TDumpStat::TUrlInfos::const_iterator toOld = oldStat.Aggregated.begin();
    TDumpStat::TUrlInfos::const_iterator toNew = newStat.Aggregated.begin();
    TDumpStat::TUrlInfos::const_iterator toOldEnd = oldStat.Aggregated.end();
    TDumpStat::TUrlInfos::const_iterator toNewEnd = newStat.Aggregated.end();
    TDumpStat::TUrlInfos diff;
    while (toOld != toOldEnd || toNew != toNewEnd) {
        if (toOld == toOldEnd) {
            Cout << UrlDescr(toNew->Url) << "\t not found in new" << Endl;
            ++toNew;
        } else if (toNew == toNewEnd) {
            Cout << UrlDescr(toOld->Url) << "\t not found in old" << Endl;
            ++toOld;
        } else if (toOld->Url < toNew->Url) {
            Cout << UrlDescr(toOld->Url) << "\t not found in old" << Endl;
            ++toOld;
        } else if (toOld->Url > toNew->Url) {
            Cout << UrlDescr(toNew->Url) << "\t not found in new" << Endl;
            ++toNew;
        } else {
            diff.push_back(TDumpStat::TUrlInfo(toOld->Url, (i64)toNew->Time - (i64)toOld->Time));
            ++toOld;
            ++toNew;
        }
    }

    Sort(diff.begin(), diff.end(), CmpByDiff);

    for (TDumpStat::TUrlInfos::const_iterator toDiff = diff.begin(); toDiff != diff.end(); ++toDiff)
        Cout << UrlDescr(toDiff->Url) << "\t" << toDiff->Time << Endl;
}

int main(int argc, char* argv[]) {
    Opt2 opts(argc, argv, "o:n:");
    TString oldDump = opts.Arg('o', "old dump");
    TString newDump = opts.Arg('n', "new dump");
    opts.AutoUsageErr();

    TDumpStat statOld(oldDump);
    TDumpStat statNew(newDump);
    Diff(statOld, statNew);

    return 0;
}
