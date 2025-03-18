#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/random/shuffle.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/split.h>


//Files from fml.yandex-team.ru
const TString QUERIES_FILE = "queries.tsv";
const TString RATINGS_FILE = "ratings.tsv";
const TString MINIMAL_RELEVANCE = "0.14";
const TString TOTAL_PAIRS = "25000";

int main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddCharOption('t', "build tasks for Turkey").NoArgument();
    opts.AddCharOption('q', "queries file").RequiredArgument("QUERIES_FILE").DefaultValue(QUERIES_FILE);
    opts.AddCharOption('r', "ratings file").RequiredArgument("RAITINGS_FILE").DefaultValue(RATINGS_FILE);
    opts.AddCharOption('m', "minimal relevance").RequiredArgument("MINIMAL_RELEVANCE").DefaultValue(MINIMAL_RELEVANCE);
    opts.AddCharOption('n', "number of qurls").RequiredArgument("TOTAL_PAIRS").DefaultValue(TOTAL_PAIRS);
    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
    bool forTurkey = optsParseResult.Has('t');
    size_t totalPairs = FromString<int>(optsParseResult.Get('n'));
    float minimalRelevance = FromString<float>(optsParseResult.Get('m'));
    TFileInput queriesFile(optsParseResult.Get('q')); // [id]\t[query]\t[region]
    TMap<TString, std::pair <TString, TString> > QueryIdToText;
    TString line;
    while (queriesFile.ReadLine(line)) {
        TVector<TString> lineData;
        StringSplitter(line).Split('\t').SkipEmpty().Collect(&lineData);
        QueryIdToText[lineData[0]] = std::make_pair(lineData[1], lineData[2]);
    }
    TVector< std::pair <TString, TString> > qdPairs;
    TFileInput ratingsFile(optsParseResult.Get('r')); // [id]\t[url]\t[relevance]
    while (ratingsFile.ReadLine(line)) {
        TVector<TString> lineData;
        StringSplitter(line).Split('\t').SkipEmpty().Collect(&lineData);
        double relevance = FromString<double>(lineData[2]);
        if (relevance >= minimalRelevance)
            qdPairs.push_back(std::make_pair(lineData[0], lineData[1]));
    }
    Shuffle(qdPairs.begin(), qdPairs.end());
    for (size_t lineNum = 0; lineNum < totalPairs; ++lineNum) {
        Cout << QueryIdToText[qdPairs[lineNum].first].first << "\t"
             << "url:" << qdPairs[lineNum].second << "\t"
             << QueryIdToText[qdPairs[lineNum].first].second;
        if (forTurkey)
            Cout << "\ttr";
        Cout << Endl;
    }
    return 0;
}
