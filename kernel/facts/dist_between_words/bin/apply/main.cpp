#include <kernel/facts/dist_between_words/trie_data.h>
#include <kernel/facts/dist_between_words/calc_factors.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/tokenizer/split.h>

#include <kernel/normalize_by_lemmas/normalize.h>
#include <quality/trailer/suggest_dict/libs/normalizer/normalizer.h>

#include <util/charset/wide.h>
#include <util/string/split.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

using namespace NUnstructuredFeatures;

int main(int argc, char** argv) {
    TString wordTriePath;
    TString gztFileName;
    TString patternsFileName;
    NLastGetopt::TOpts opts;
    opts.SetTitle("Calculate distance between queries based on word distances");
    opts.AddCharOption('t', "Word distance trie. Take it from https://a.yandex-team.ru/arc/trunk/arcadia/quality/functionality/entity_search/factqueries/online_alias_resources/ya.make?rev=4283060#L19")
        .Required()
        .StoreResult(&wordTriePath);
    opts.AddLongOption("gzt", "Special words gzt")
            .StoreResult(&gztFileName);
    opts.AddLongOption("patterns", "all_patterns.txt")
            .StoreResult(&patternsFileName);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto normalizeInfo = TNormalizeByLemmasInfo(gztFileName, patternsFileName);

    auto wordDistTrie = MakeAtomicShared<NDistBetweenWords::TWordDistTrie>(TBlob::FromFile(wordTriePath));
    TUtf16String line;
    auto calcer = NDistBetweenWords::TCalcFactors(wordDistTrie);
    auto factors = {FI_WORD_DIST_URL_5_NORM_QUERY, FI_WORD_DIST_URL_10_NORM_QUERY,
                    FI_WORD_DIST_HOST_5_NORM_QUERY, FI_WORD_DIST_HOST_10_NORM_QUERY,
                    FI_WORD_DIST_URL_5_NORM_NORM_QUERY, FI_WORD_DIST_URL_10_NORM_NORM_QUERY,
                    FI_WORD_DIST_HOST_5_NORM_NORM_QUERY, FI_WORD_DIST_HOST_10_NORM_NORM_QUERY};
    while (Cin.ReadLine(line)) {
        TVector<TUtf16String> parts = StringSplitter(line).Split(wchar16('\t'));
        Y_ENSURE(parts.size() == 2);
        NUnstructuredFeatures::TFactFactorStorage features;
        TUtf16String normQuery1 =  NormalizeByLemmas(parts[0], normalizeInfo);
        TUtf16String normQuery2 = NormalizeByLemmas(parts[1], normalizeInfo);
        TVector<TUtf16String> query1Words = SplitIntoTokens(normQuery1, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        TVector<TUtf16String> query2Words = SplitIntoTokens(normQuery2, TTokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT));
        calcer.CalculateNormalized(query1Words, query2Words, features);
        for (auto factor : factors) {
            Cout << features[factor] << " ";
        }
        Cout << Endl;
    }
}
