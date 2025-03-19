#include "options.h"

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <ysite/yandex/pure/pure.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <kernel/lemmer/core/language.h>
#include <quality/trailer/suggest_dict/libs/normalizer/normalizer.h>

#include <util/string/join.h>
#include <util/string/strip.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>

#include <utility>

using namespace NYT;
using namespace NDistBetweenWords;

namespace {
    float CalcLangReverseFreq(const TUtf16String& word, const TPure& pure, ELanguage lang) {
        TPureBase::TPureData pureData = pure.GetByForm(word, NPure::AllCase, lang);
        return pureData.GetFreq();
    }

    float CalcReverseFreq(const TUtf16String& word, const TPure& pure) {
        float rf = CalcLangReverseFreq(word, pure, LANG_RUS);
        rf = Max(rf, CalcLangReverseFreq(word, pure, LANG_ENG));
        rf = Max(rf, CalcLangReverseFreq(word, pure, LANG_UNK));
        return rf;
    }
}


class TFilterByFreq: public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    TFilterByFreq() = default;

    TFilterByFreq(TString pureFile, int threshold, TString searchSynTrieFile) :
            PureFile(std::move(pureFile)),
            Threshold(threshold),
            SearchSynTrieFile(std::move(searchSynTrieFile))
    {}

    Y_SAVELOAD_JOB(PureFile, Threshold, SearchSynTrieFile);

    void Start(TWriter*) override {
        PurePtr.Reset(new TPure(PureFile));
        if (!!SearchSynTrieFile) {
            SearchSynTrie = TCompactTrie<wchar16, bool>(TBlob::FromFile(SearchSynTrieFile));
        }
    }

    void Do(TReader* input, TWriter* output) override {
        TString delim = " ";
        auto&& langs = TLangMask(LANG_RUS, LANG_ENG);
        for (; input->IsValid(); input->Next()) {
            TNode row = input->GetRow();
            auto& query = row["query"].AsString();
            auto norm_query = Normalize(query);
            TVector<TString> words;
            Split(norm_query, delim, words);
            if (norm_query.length() < 1000 && words.size() > 1) {
                words.push_back("");
                for (auto i : xrange(words.size())) {
                    double number;
                    TString word = words[i];
                    TUtf16String wideWord = CharToWide(word, CODES_UTF8);
                    TUtf16String normalizeWord = NormalizeByLemma(wideWord, langs);
                    const auto freq = !word.empty() ?
                                      CalcReverseFreq(wideWord, *PurePtr) :
                                      std::numeric_limits<float>::max();
                    if (!TryFromString(word, number) &&
                    (freq >= Threshold || (SearchSynTrie.IsInitialized() && SearchSynTrie.Find(normalizeWord)))) {
                        TVector<TString> keyWords(words);
                        keyWords.erase(keyWords.begin() + i);
                        auto key = Strip(JoinSeq(delim, keyWords));
                        output->AddRow(TNode()
                            ("key", key)
                            ("word", word)
                            ("pos", word.empty() ? -1 : i)
                            ("hosts", row["hosts"])
                            ("urls", row["urls"])
                            ("freq", freq))
                            ;

                    }
                    if (words.size() > 3 && i < words.size() - 1) {
                        TString nextWord = words[i + 1];
                        if (!nextWord.empty()) {
                            TUtf16String nextWideWord = CharToWide(nextWord, CODES_UTF8);
                            TUtf16String normalizeNextWord = NormalizeByLemma(nextWideWord, langs);
                            const auto nextFreq = CalcReverseFreq(nextWideWord, *PurePtr);
                            if (!TryFromString(nextWord, number) &&
                            ((nextFreq >= Threshold && freq >=Threshold) ||
                            (SearchSynTrie.IsInitialized() && SearchSynTrie.Find(normalizeWord + wchar16(' ') + normalizeNextWord)))) {
                                TVector<TString> bigramKeyWords(words);
                                bigramKeyWords.erase(bigramKeyWords.begin() + i, bigramKeyWords.begin() + i + 2);
                                auto bigramKey = Strip(JoinSeq(delim, bigramKeyWords));
                                output->AddRow(TNode()
                                                       ("key", bigramKey)
                                                       ("word", word + delim + nextWord)
                                                       ("pos", i)
                                                       ("hosts", row["hosts"])
                                                       ("urls", row["urls"])
                                                       ("freq", std::min(freq, nextFreq)));
                            }
                        }
                    }
                }
            }
        }
    }

private:
    TString Normalize(const TString& query) {
        auto w = UTF8ToWide(query);
        TUtf16String resCorr;
        Normalizer.NormalizeSpaces(w);
        Normalizer.NormalizePunctuation(w, resCorr);
        return WideToUTF8(w);
    }

    TUtf16String NormalizeByLemma(const TUtf16String& word, const TLangMask& langs) {
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(word.c_str(), word.length(), lemmas, langs);
        if (lemmas.empty()) {
            return word;
        } else {
            return lemmas[0].GetText();
        }
    }

    TString PureFile;
    THolder<TPure> PurePtr;
    TSuggestNormalizer Normalizer;
    int Threshold;
    TString SearchSynTrieFile;
    TCompactTrie<wchar16, bool> SearchSynTrie;
};


REGISTER_MAPPER(TFilterByFreq);



int main(int argc, const char** argv) {
    Initialize(argc, argv);

    TOptions opts(argc, argv);

    IClientPtr client = CreateClient("hahn");

    TMapOperationSpec spec;
    TUserJobSpec userSpec;
    userSpec.AddLocalFile(opts.PureFile);
    if (!!opts.SearchSynTrie) {
        userSpec.AddLocalFile(opts.SearchSynTrie);
    }
    spec.MapperSpec(userSpec);
    spec.AddInput<TNode>(opts.InputTable)
            .AddOutput<TNode>(opts.OutputTable);
    client->Map(spec, new TFilterByFreq(opts.PureFile, opts.Threshold, opts.SearchSynTrie));
    client->Sort(TSortOperationSpec()
         .AddInput(opts.OutputTable)
         .Output(opts.OutputTable)
         .SortBy("key"),
         TOperationOptions()
    );

//    TPure pure(opts.PureFile);
//    TVector<TString> examples = {
//            "привет",
//            "как",
//            "в",
//            "кайенн",
//            "gipfel",
//            "росэлектроника",
//            "newobrazovanie",
//            "ackac",
//            "киевский",
//            "киевское"
//    };
//
//    for (auto& text : examples) {
//        auto word = CharToWide(text, CODES_UTF8);
//        float rf = CalcReverseFreq(word, pure);
//        Cout << text << " = " << rf << Endl;
//    }
}




