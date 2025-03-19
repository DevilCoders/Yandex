#include <kernel/title_ranges/lib/title_ranges.h>
#include <kernel/title_ranges/calcer.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/string/split.h>

int main(int argc, const char* argv[])
{
    NLastGetopt::TOpts options;

    options.AddLongOption("profile", "Dump profiling data to stdout")
        .NoArgument();

    options.SetFreeArgsMin(1);
    options.SetFreeArgsMax(1);

    options.SetFreeArgTitle(0, "<input-tsv>", "TSV file with records (query, title)");

    NLastGetopt::TOptsParseResult res(&options, argc, argv);
    TVector<TString> freeArgs = res.GetFreeArgs();

    TString inputPath = freeArgs[0];
    bool profile = res.Has("profile");

    TDocTitleRangesCalcer calcer(nullptr);

    TDuration timeCalcAll = TDuration::MicroSeconds(0);
    TDuration timeCalcQueries = TDuration::MicroSeconds(0);
    TDuration timeCalcDocs = TDuration::MicroSeconds(0);
    ui32 numLines = 0;
    ui32 numQueryMatches = 0;
    ui32 numDocMatches = 0;
    ui32 docBytes = 0;
    ui32 queryBytes = 0;
    TProfileTimer timerMain;

    TFileInput inp(inputPath);
    TString line;
    while (inp.ReadLine(line)) {
        StripInPlace(line);
        TVector<TStringBuf> fields;
        StringSplitter(line.data(), line.data() + line.size()).Split('\t').AddTo(&fields);

        if (fields.size() < 2) {
            Cerr << "-E- Bad input line: \"" << line << "\"" << Endl;
            return 2;
        }

        TStringBuf query = fields[0];
        TStringBuf title = fields[1];
        TLangMask langMask;

        if (fields.size() >= 3) {
            langMask = TLangMask(static_cast<ELanguage>(FromString<int>(fields[2])));
        }

        queryBytes += query.size();
        docBytes += title.size();

        TUtf16String wQuery, wTitle;

        try {
            wQuery = UTF8ToWide(query);
            wTitle = UTF8ToWide(title);
        }
        catch (...) {
            Cerr << "-E- Failed to decode UTF8: \"" << line << "\"" << Endl;
            return 2;
        }

        TProfileTimer timerCalc;

        TDocTitleRanges queryRanges, titleRanges;

        TProfileTimer timerCalcQuery;
        bool matchQuery = calcer.CalcRanges(wQuery, queryRanges, langMask);
        TDuration timeCalcQuery = timerCalcQuery.Get();

        TProfileTimer timerCalcDoc;
        bool matchDoc = calcer.CalcRanges(wTitle, titleRanges, langMask);
        TDuration timeCalcDoc = timerCalcDoc.Get();

        TDuration timeCalc = timerCalc.Get();

        float classScore = GetTitleRangesClassScore(queryRanges);
        float matchScore = GetTitleRangesMatchingScore(queryRanges, titleRanges);

        Cout << line << "\t" << ToString(classScore)
             << "\t" << ToString(matchScore) << Endl;

        numLines += 1;
        numQueryMatches += matchQuery ? 1 : 0;
        numDocMatches += matchDoc ? 1 : 0;
        timeCalcDocs += timeCalcDoc;
        timeCalcQueries += timeCalcQuery;
        timeCalcAll += timeCalc;
    }

    TDuration timeMain = timerMain.Get();

    if (profile) {
        Cout << "-P- Time in main loop (microsec): " << timeMain.MicroSeconds() << Endl;
        Cout << "-P- Number of lines processed: " << numLines << Endl;
        Cout << "-P- Total time to calc all ranges (microsec): " << timeCalcAll.MicroSeconds() << Endl;

        Cout << "-P- Total number of query bytes: " << queryBytes << Endl;
        Cout << "-P- Number of matching queries found: " << numQueryMatches << Endl;
        Cout << "-P- Total time to calc query ranges (microsec): " << timeCalcQueries.MicroSeconds() << Endl;
        Cout << "-P- Average time to calc query ranges per doc (microsec): " << (double) timeCalcQueries.MicroSeconds() / (double) numLines << Endl;

        Cout << "-P- Total number of doc bytes: " << docBytes << Endl;
        Cout << "-P- Number of matching docs found: " << numDocMatches << Endl;
        Cout << "-P- Total time to calc doc ranges (microsec): " << timeCalcDocs.MicroSeconds() << Endl;
        Cout << "-P- Average time to calc doc ranges per doc (microsec): " << (double) timeCalcDocs.MicroSeconds() / (double) numLines << Endl;
    }

    return 0;
}

