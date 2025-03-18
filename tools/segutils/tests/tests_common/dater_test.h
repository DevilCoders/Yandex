#pragma once

#include "tests_common.h"

namespace NSegutils {

struct TDaterTest: TTest {

    THolder<TDaterContext> DaterCtx;

    bool UseDater2;
    ND2::EDaterMode Mode;
    bool Benchmark;

    TDaterTest()
        : UseDater2()
        , Mode(ND2::DM_MAIN_DATES)
        , Benchmark()
    { }

    void DoInit() override {
        DaterCtx.Reset(new TDaterContext(*Ctx));
        DaterCtx->SetUseDater2(UseDater2, Mode);
    }

    bool ProcessArg(int arg, Opt& o) override {
        switch (arg) {
        case 'i':
            Reader.CommonTime.SetDate(ScanDateSimple(o.GetArg()));
            return true;
        case 'b':
            Benchmark = true;
            return true;
        case 'M':
            if (!strcmp("md", o.GetArg()))
                Mode = ND2::DM_MAIN_DATES;
            else if (!strcmp("mdmr", o.GetArg()))
                Mode = ND2::DM_MAIN_DATES_MAIN_RANGES;
            else if (!strcmp("ad", o.GetArg()))
                Mode = ND2::DM_ALL_DATES;
            else if (!strcmp("adar", o.GetArg()))
                Mode = ND2::DM_ALL_DATES_ALL_RANGES;
            else if (!strcmp("admr", o.GetArg()))
                Mode = ND2::DM_ALL_DATES_MAIN_RANGES;
            else
                return false;
            return true;
        }

        return false;
    }

    TString GetHelp() const override {
        return " -i <indexed_date> [-M <md|mdmr|ad|adar|admr> -b] ";
    }

    TString GetArgs() const override {
        return "i:M:b";
    }

    void SetUseDater2(bool dater2) {
        UseDater2 = dater2;
    }

    TString ProcessDoc(const THtmlFile& f) override {
        using namespace NDater;
        using namespace NSegm;
        TDaterContext& ctx = *DaterCtx;
        ctx.SetDoc(f);
        ctx.NumerateDoc();

        if (Benchmark)
            return TString();

        TEventStorage& title = ctx.GetEvents(true);

        {
            const TDatePositions& td = ctx.GetTitleDates();
            for (TDatePositions::const_iterator it = td.begin(); it != td.end(); ++it)
                title.InsertSpan(*it, "Date " + it->ToString());
        }

        TEventStorage& body = ctx.GetEvents(false);

        {
            const TDatePositions& td = ctx.GetBodyDates();
            for (TDatePositions::const_iterator it = td.begin(); it != td.end(); ++it)
                body.InsertSpan(*it, "Date " + it->ToString());
        }

        {
            const THeaderSpans& hs = ctx.GetHeaderSpans();
            for (THeaderSpans::const_iterator it = hs.begin(); it != hs.end(); ++it)
                body.InsertSpan(*it, "Header");
        }

        {
            const TMainHeaderSpans& hs = ctx.GetMainHeaderSpans();
            for (TMainHeaderSpans::const_iterator it = hs.begin(); it != hs.end(); ++it)
                body.InsertSpan(*it, "MainHeader");
        }

        {
            const TSegmentSpans& ss = ctx.GetSegmentSpans();
            for (TSegmentSpans::const_iterator it = ss.begin(); it != ss.end(); ++it)
                body.InsertSpan(*it, Sprintf("Segment-%s", GetSegmentName(ESegmentType(it->Type))));
        }

        {
            const TMainContentSpans& ms = ctx.GetMainContentSpans();
            for (TMainContentSpans::const_iterator it = ms.begin(); it != ms.end(); ++it)
                body.InsertSpan(*it, "MainContent");
        }

        {
            const TArticleSpans& as = ctx.GetArticleSpans();
            for (TArticleSpans::const_iterator it = as.begin(); it != as.end(); ++it)
                body.InsertSpan(*it, "Article");
        }

        TString res;

        TStringOutput sout(res);
        sout << "<-------------------------------------------------->" << '\n';
        sout << "Url: " << f.Url << '\n';
        sout << "File: " << TFsPath(f.FileName).GetName() << '\n';
        sout << "OrigDate: " << f.RealDate.ToString() << '\n';
        sout << "BestDate: " << ctx.GetBestDate().ToString() << '\n';
        sout << "DateStatsDM: " << ctx.GetDaterStats().ToStringDaysMonths() << '\n';
        sout << "DateStatsMY: " << ctx.GetDaterStats().ToStringMonthsYears() << '\n';
        sout << "DateStatsY: " << ctx.GetDaterStats().ToStringYears() << '\n';
        sout << "DateStatsDMFiltered: " << ctx.GetDaterStatsFiltered().ToStringDaysMonths() << '\n';
        sout << "DateStatsMYFiltered: " << ctx.GetDaterStatsFiltered().ToStringMonthsYears() << '\n';
        sout << "DateStatsYFiltered: " << ctx.GetDaterStatsFiltered().ToStringYears() << '\n';
        sout << "TopDates: " << SaveDatesList(ctx.GetTopDates()) << '\n';
        sout << "UrlDates: " << SaveDatesList(ctx.GetUrlDates()) << '\n';

        sout << "<Title>" << '\n';
        PrintEvents(ctx.GetEvents(true), sout);
        sout << "</Title>" << '\n';;
        sout << "<Body>" << '\n';
        PrintEvents(body, sout);
        sout << "</Body>" << '\n';
        sout << Endl;
        return res;
    }
};

}
