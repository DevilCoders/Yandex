#include <tools/segutils/segcommon/data_utils.h>

#include <kernel/segutils/numerator_utils.h>

#include <library/cpp/getopt/opt.h>

#include <util/system/winint.h>

#include <cmath>

using namespace NDater;

namespace NSegutils {

enum EStatus {
    S_OK = 0, S_NO_DATE, S_ERROR, S_NOT_FOUND, S_NOT_SKIPPED, S_WRONG, S_COUNT
};

struct TMissSettings {
    ui32 UndefPenalty;
    ui32 Threshold;
    bool InMonths;

    TMissSettings()
        : UndefPenalty()
        , Threshold()
        , InMonths()
    {}
};

struct TMiss {
    EStatus Status;
    bool HadAnyChance;

    TDaterDate Real;
    TDaterDate Best;

    TMissSettings Settings;

public:
    explicit TMiss(EStatus status = S_ERROR)
        : Status(status)
        , HadAnyChance()
    {
    }

    TMiss(TDaterDate real, TDaterDate best, TMissSettings settings)
        : Status(S_ERROR)
        , HadAnyChance()
        , Real(real)
        , Best(best)
        , Settings(settings)
    {
        bool rv = real ;// && !real.YearOnly();
        bool bv = best ;// && !best.YearOnly();

        Status = (rv && bv) ? S_OK : (rv && !bv) ? S_NOT_FOUND : (!rv && bv) ? S_NOT_SKIPPED : S_NO_DATE;
    }

    void CheckChance(TDaterDate date) {
        if (CheckDate(date))
            HadAnyChance = true;
    }

    bool CheckBest() {
        if (S_OK == Status && !CheckDate(Best))
            Status = S_WRONG;

        return IsWrong();
    }

    bool IsError() const { return S_ERROR == Status; }
    bool IsNotFound() const { return S_NOT_FOUND == Status; }
    bool IsNotSkipped() const { return S_NOT_SKIPPED == Status; }
    bool IsNoDate() const { return S_NO_DATE == Status; }
    bool IsWrong() const { return S_WRONG == Status; }
    bool IsOk() const { return S_OK == Status; }

    TString ReportStatus() const {
        switch(Status) {
        default: Y_FAIL(" ");
        case S_NOT_SKIPPED: return "NotSkipped";
        case S_NOT_FOUND: return "NotFound";
        case S_NO_DATE: return "NoDate";
        case S_ERROR: return "Error";
        case S_WRONG: return "Wrong";
        case S_OK: return "Ok";
        }
    }

    bool CheckDate(TDaterDate date) {
        if (S_OK != Status && !IsWrong() || date.NoYear) // todo: check date.Year properly
            return false;

        ui32 absdiffdays = -1;
        ui32 absdiffmonths = -1;

        if (!Real.Month) {
            absdiffdays   = 365 * abs((i32) Real.Year - (i32) date.Year) + 30.5 * bool(date.Month) * Settings.UndefPenalty;
            absdiffmonths =  12 * abs((i32) Real.Year - (i32) date.Year) +        bool(date.Month) * Settings.UndefPenalty;
        } else if (!Real.Day) {
            absdiffdays   = 30.5 * abs((i32) Real.GetMonthsAD() - (i32) date.GetMonthsAD()) + bool(date.Day) * Settings.UndefPenalty;
            absdiffmonths =        abs((i32) Real.GetMonthsAD() - (i32) date.GetMonthsAD());
        } else if (!date.Month) {
            absdiffdays   = 365 * abs((i32) Real.Year - (i32) date.Year) + 30.5 * Settings.UndefPenalty;
            absdiffmonths =  12 * abs((i32) Real.Year - (i32) date.Year) +    1 * Settings.UndefPenalty;
        } else if (!date.Day) {
            absdiffdays   = 30.5 * abs((i32) Real.GetMonthsAD() - (i32) date.GetMonthsAD()) + 1 * Settings.UndefPenalty;
            absdiffmonths =        abs((i32) Real.GetMonthsAD() - (i32) date.GetMonthsAD());
        } else {
            absdiffdays   = abs((i32) Real.GetDaysAD()   - (i32) date.GetDaysAD());
            absdiffmonths = abs((i32) Real.GetMonthsAD() - (i32) date.GetMonthsAD());
        }

        ui32 diff = Settings.InMonths ? absdiffmonths : absdiffdays;

        return diff <= Settings.Threshold;
    }
};

struct TDateCounter {
    ui32 Counts[S_COUNT];
    ui32 HadChance;

public:
    TDateCounter()
        : Counts()
        , HadChance()
    {}

    void Add(TMiss miss) {
        ++Counts[miss.Status];
        HadChance += miss.HadAnyChance;
    }

    TString ReportHeadline(const char * label = "") const {
        return Sprintf("%27s tr.pos     ch    all      Pr      Re     F1      Sh      Ch", label);
    }

    TString Report(const char * label, ui32 all) const {
        return Sprintf("%26s: %6u %6u %6u %6.1f%% %6.1f%% %6.3f %6.1f%% %6.1f%%",
                       label,
                       Ok(),
                       HadChance,
                       TotalDated(),
                       Pr() * 100,
                       Re(all) * 100,
                       F1(all),
                       (all ? float(TotalDated()) / all : 1) * 100,
                       (all ? float(HadChance) / all : 1) * 100);
    }

    ui32 Ok() const { return Counts[S_OK]; }
    ui32 Wrong() const { return Counts[S_WRONG]; }
    ui32 NotFound() const { return Counts[S_NOT_FOUND]; }

    ui32 NotSkipped() const { return Counts[S_NOT_SKIPPED]; }
    ui32 NoDate() const { return Counts[S_NO_DATE]; }

    float Pr()         const { return TotalDated() ? float(Ok()) / TotalDated() : 1.; }
    float Re(ui32 all) const { return all ? float(Ok()) / all : 1; }
    float F1(ui32 all) const { return Pr() && Re(all) ? 2 * Pr() * Re(all) / (Pr() + Re(all)) : 0; }

    ui32 TotalDateable() const { return Ok() + Wrong() + NotFound(); }
    ui32 TotalDated() const { return Ok() + Wrong() + NotSkipped(); }
};

struct TDQContext {
    THolder<TParserContext> Ctx;
    THolder<TDaterContext> Dater;
    TFileList List;
    THtmlFileReader Reader;

    TMissSettings Settings;

public:
    ui32 Count;
    bool ShowBestIrrel;
    bool UseDater2;
    ND2::EDaterMode Dater2Mode;

public:
    static TDaterDate GetDefaultIndexedDate() {
        return TDaterDate(2009, 6, 30);
    }

    TDQContext(int argc, const char** argv)
        : Count(Max<ui32> ())
        , ShowBestIrrel()
        , UseDater2()
    {
        if (argc < 2 || argc > 1 && strcmp(argv[1], "--help") == 0)
            UsageAndExit(argv[0]);

        Opt opts(argc, argv, "c:f:t:T:n:m:i:d:2:b");
        int optlet;

        TString dir;
        TMapping meta;
        Reader.MetaData.RealDate = -1;

        while (EOF != (optlet = opts.Get())) {
            switch (optlet) {
            case 'i':
                Reader.CommonTime.SetDate(ScanDateSimple(opts.GetArg()));
                break;
            case 'c':
                Ctx.Reset(new TParserContext(opts.GetArg()));
                break;
            case 'f':
                dir = opts.GetArg();
                Reader.SetDirectory(dir);
                break;
            case 't':
                Settings.Threshold = FromString<ui32> (opts.GetArg());
                Settings.InMonths = false;
                break;
            case 'T':
                Settings.Threshold = FromString<ui32> (opts.GetArg());
                Settings.InMonths = true;
                break;
            case 'n':
                Count = FromString<ui32> (opts.GetArg());
                break;
            case 'd':
                Settings.UndefPenalty = FromString<ui32> (opts.GetArg());
                break;
            case 'm':
                Reader.InitMapping(opts.GetArg());
                break;
            case 'b':
                ShowBestIrrel = true;
                break;
            case '2':
                UseDater2 = true;
                if (!strcmp("md", opts.GetArg()))
                    Dater2Mode = ND2::DM_MAIN_DATES;
                else if (!strcmp("mdmr", opts.GetArg()))
                    Dater2Mode = ND2::DM_MAIN_DATES_MAIN_RANGES;
                else if (!strcmp("ad", opts.GetArg()))
                    Dater2Mode = ND2::DM_ALL_DATES;
                else if (!strcmp("adar", opts.GetArg()))
                    Dater2Mode = ND2::DM_ALL_DATES_ALL_RANGES;
                else if (!strcmp("admr", opts.GetArg()))
                    Dater2Mode = ND2::DM_ALL_DATES_MAIN_RANGES;
                else
                    UsageAndExit(argv[0]);

                break;

            default:
                UsageAndExit(argv[0]);
            }
        }

        List.Fill(dir);

        if (!Ctx || !Count || !Reader.CommonTime.Timestamp)
            UsageAndExit(argv[0]);
        Dater.Reset(new TDaterContext(*Ctx));
    }

    static void UsageAndExit(const char * me) {
        Cerr << me << " -c <configDir> -f <fileDir> -t <threshold in days> -T <threshold in months> [-n <count>] [-m <metadata>] [-i <date>] [-d]"
                << Endl;
        Cerr << "\t-c <configDir>: dir with htparser.ini, dict.dict and 2ld.list" << Endl;
        Cerr << "\t-f <fileDir>: dir with zipped files" << Endl;
        Cerr << "\t-t <threshold in days>: error threshold in days (diffs gt this num are treated as miss)"
                << Endl;
        Cerr << "\t-T <threshold in months>: same in months. Error in days is ignored" << Endl;
        Cerr << "\t-n <count>: max docs to process, default is all" << Endl;
        Cerr << "\t-m <metadata>: use file with additional metadata" << Endl;
        Cerr << "\t-i <date>: reference index data, default is " << GetDefaultIndexedDate().ToString(
                "%d/%m/%Y") << Endl;
        Cerr << "\t-d <undef penalty> add a penalty for incomplete dates" << Endl;
        Cerr << "\t-b dump urls for docs with very trusted dates having no real date" << Endl;
        Cerr << "\t-2 <md|mdmr|ad|adar|admr> use dater2 mode" << Endl;
        exit(0);
    }
};

typedef TVector<TMiss> TMisses;

struct TDatedUrl {
    TString Url;
    TDaterDate Date;

    TDatedUrl()
    {}

    TDatedUrl(const TString& u, TDaterDate d)
        : Url(u)
        , Date(d)
    {}
};

typedef TVector<TDatedUrl> TDatedUrls;

const ui32 BEST_DATE_MISS = 0;

void CheckDate(const THtmlFile& data, TDQContext& ctx, TDatedUrls& bestirrel, TMisses& misses) {
    try {
        TDaterContext& dc = *ctx.Dater;
        dc.SetUseDater2(ctx.UseDater2, ctx.Dater2Mode);
        dc.SetDoc(data);
        dc.NumerateDoc();

        misses.clear();
        misses.resize(TDaterDate::FromsCount);

        TDatePositionsBySource dpss;
        TDatePositions bdss;
        const TDatePositions& dps = dc.GetAllDates();
        SortDatesBySources(dpss, dps);
        FindBestDateFromSorted(bdss, dpss, data.Time.Date);

        TDatePosition best = dc.GetBestDate();
//        TDatePosition best2 = segctx.GetSecondBestDate();

        TMiss& bestmiss = misses[BEST_DATE_MISS];
        bestmiss = TMiss(data.RealDate, best, ctx.Settings);
        bestmiss.CheckBest();

        for (TDatePositions::const_iterator it = dps.begin(); it != dps.end(); ++it)
            bestmiss.CheckChance(*it);

        if (!data.RealDate && best.IsVeryTrusted())
            bestirrel.push_back(TDatedUrl(data.Url, best));

        for (ui32 i = TDaterDate::FromUnknown + 1; i < TDaterDate::FromsCount; ++i) {
            Y_VERIFY(!bdss[i] || bdss[i].From == i, "%u %u", bdss[i].From, i);
            misses[i] = TMiss(data.RealDate, bdss[i], ctx.Settings);
            misses[i].CheckBest();

            for (TDatePositions::const_iterator it = dpss[i].begin(); it != dpss[i].end(); ++it)
                misses[i].CheckChance(*it);
        }
    } catch (const yexception& e) {
        Cerr << "exception: " << e.what() << Endl;
        misses.clear();
        misses.push_back(TMiss(S_ERROR));
    }
}

}

int main(int argc, const char**argv) {
    using namespace NSegutils;
    TDQContext ctx(argc, argv);

    TDateCounter from[TDaterDate::FromsCount];
    TDateCounter fromby[TDaterDate::FromsCount];
    TDateCounter    totalnofooter,
                    fromurlall,
                    frombyurlall,
                    fromaroundmainall,
                    frombyaroundmainall,
                    frommainendsall,
                    frombymainendsall,
                    fromtextall,
                    frombytextall;

    TDateCounter& total = from[0];

    ui32 n = 0;
    ui32 gtot = 0;
    const char* fname = nullptr;

    TDatedUrls bestirrel;
    TMisses misses;

    while (n < ctx.Count && (fname = ctx.List.Next())) {
        THtmlFile f = ctx.Reader.Read(fname);

        CheckDate(f, ctx, bestirrel, misses);

        TMiss& best = misses[BEST_DATE_MISS];

        if (best.IsError())
            continue;

        Clog << ++n << "\t" << f.FileName << Endl;
        total.Add(best);

        if (best.IsNoDate())
            continue;

        Cout << best.ReportStatus() << "\t" << f.RealDate.ToString("%d/%m/%Y") << "\t" << best.Best.ToString() << "\t" << f.Url << Endl;

        ++gtot;

        for (ui32 i = TDaterDate::FromUnknown + 1; i < TDaterDate::FromsCount; ++i) {
            TMiss m = misses[i];
            if (m.Best) {
                Y_VERIFY(m.Best.From == i, "%u != %u", m.Best.From, i);

                from[i].Add(m);

                switch(m.Best.From) {
                case TDaterDate::FromUrl:
                case TDaterDate::FromUrlId:
                    fromurlall.Add(m);
                    break;
                case TDaterDate::FromBeforeMainContent:
                case TDaterDate::FromAfterMainContent:
                    fromaroundmainall.Add(m);
                    break;
                case TDaterDate::FromMainContentStart:
                case TDaterDate::FromMainContentEnd:
                    frommainendsall.Add(m);
                    break;
                case TDaterDate::FromContent:
                case TDaterDate::FromText:
                    fromtextall.Add(m);
                    break;
                }
            }
        }

        if (best.Best) {
            fromby[best.Best.From].Add(best);

            switch(best.Best.From) {
            case TDaterDate::FromUrl:
            case TDaterDate::FromUrlId:
                frombyurlall.Add(best);
                break;
            case TDaterDate::FromBeforeMainContent:
            case TDaterDate::FromAfterMainContent:
                frombyaroundmainall.Add(best);
                break;
            case TDaterDate::FromMainContentStart:
            case TDaterDate::FromMainContentEnd:
                frombymainendsall.Add(best);
                break;
            case TDaterDate::FromContent:
            case TDaterDate::FromText:
                frombytextall.Add(best);
                break;
            }

            if (best.Best.From != TDaterDate::FromFooter)
                totalnofooter.Add(best);
        }
    }

    Cout << total.ReportHeadline() << Endl;
    Cout << from[TDaterDate::FromUrl].Report("url path", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromUrlId].Report("url id", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromTitle].Report("title", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromBeforeMainContent].Report("before main", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromMainContentStart].Report("main start", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromMainContent].Report("main", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromMainContentEnd].Report("main end", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromAfterMainContent].Report("after main", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromContent].Report("content", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromText].Report("text", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromFooter].Report("footer", total.TotalDateable()) << Endl;
    Cout << Endl;

    Cout << fromby[TDaterDate::FromUrl].Report("url path (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromUrlId].Report("url id (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromTitle].Report("title (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromBeforeMainContent].Report("before main (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromMainContentStart].Report("main start (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromMainContent].Report("main (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromMainContentEnd].Report("main end (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromAfterMainContent].Report("after main (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromContent].Report("content (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromText].Report("text (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromFooter].Report("footer (in best)", total.TotalDateable()) << Endl;

    Cout << total.Report("best", total.TotalDateable()) << Endl;
    Cout << Endl;

    Cout << "Groups: " << Endl << Endl;

    Cout << fromurlall.Report("all url", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromTitle].Report("title", total.TotalDateable()) << Endl;
    Cout << fromaroundmainall.Report("around main", total.TotalDateable()) << Endl;
    Cout << frommainendsall.Report("main ends", total.TotalDateable()) << Endl;
    Cout << from[TDaterDate::FromMainContent].Report("main content", total.TotalDateable()) << Endl;
    Cout << fromtextall.Report("content and text", total.TotalDateable()) << Endl;
    Cout << Endl;

    Cout << frombyurlall.Report("all url (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromTitle].Report("title (in best)", total.TotalDateable()) << Endl;
    Cout << frombyaroundmainall.Report("around main (in best)", total.TotalDateable()) << Endl;
    Cout << frombymainendsall.Report("main ends (in best)", total.TotalDateable()) << Endl;
    Cout << fromby[TDaterDate::FromMainContent].Report("main content (in best)", total.TotalDateable()) << Endl;
    Cout << frombytextall.Report("content and text (in best)", total.TotalDateable()) << Endl;
    Cout << Endl;

    Cout << totalnofooter.Report("best (no footer)", total.TotalDateable()) << Endl;
    Cout << Endl;

    Cout << "Not found " << total.NotFound() << " of " << total.TotalDateable()
                    << " (" << Sprintf("%.1f%%", total.TotalDateable() ?
                                    100 * float(total.NotFound()) / total.TotalDateable() : 100) << ")" << Endl;
    Cout << "Failed to skip " << total.NotSkipped() << " of " << total.NoDate()
                    << " (" << Sprintf("%.1f%%", total.NoDate() ?
                                    100 * float(total.NotSkipped()) / total.NoDate() : 0) << ")" << Endl;
    Cout << Endl;

    Cout << "     'Pr' shows the share of the correct dates " << Endl;
    Cout << "          among all the dates comming from the source." << Endl;
    Cout << "     'Re' compares the share of the correct dates from the source " << Endl;
    Cout << "          to the amount of all the documents with a sensible real date." << Endl;

    if (ctx.ShowBestIrrel) {
        Cout << "Docs without real dates having very trusted dates:" << Endl;
        for (TDatedUrls::const_iterator it = bestirrel.begin(); it != bestirrel.end(); ++it)
            Cout << it->Url << "\t" << it->Date.ToString() << Endl;
    }

    return 0;
}

