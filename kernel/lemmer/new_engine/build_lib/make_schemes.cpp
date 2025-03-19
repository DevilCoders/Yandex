#include "common.h"

#include <util/string/builder.h>
#include <util/generic/string.h>
#include <util/string/join.h>
#include <util/digest/multi.h>
#include <util/generic/set.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/string/subst.h>

namespace {
    //who even cares about performance?
    //todo: make all processing in Utf32
    using NDictBuild::StringToUtf32;
    using NDictBuild::TParseConstants;
    using NDictBuild::TUsingWideStringType;

    struct TForm {
        TUsingWideStringType Text;
        TVector<TString> Grams;
        TForm(const TUsingWideStringType& text, const TVector<TString>& grams)
            : Text(text)
            , Grams(grams)
        {
        }
    };
    struct TChunk {
        TUsingWideStringType Lemma;
        TVector<TForm> Forms;
        TChunk(const TUsingWideStringType& lemma, const TVector<TForm>& forms)
            : Lemma(lemma)
            , Forms(forms)
        {
        }
    };

    TUsingWideStringType Norm(const TUsingWideStringType& s) {
        TUsingWideStringType result = s;
        ToLower(result);
        return result;
    }

    TVector<TChunk> ListerChunk(IInputStream& in) {
        TUsingWideStringType lemma;
        TVector<TForm> forms;
        TVector<TChunk> result;
        TString line;
        while (in.ReadLine(line)) {
            line = StripStringRight(line);
            if (line.empty()) {
                if (forms.empty()) {
                    lemma = TParseConstants::Empty;
                } else {
                    result.emplace_back(lemma, forms);
                }
            }
            if (line.find('[') == TString::npos) {
                if (!forms.empty() && !lemma.empty()) {
                    result.emplace_back(lemma, forms);
                }
                lemma = Norm(StringToUtf32(line));
                forms.clear();
            } else {
                SubstGlobal(line, '\t', ' ');
                size_t spacePos = line.find_last_of(' ');
                Y_ENSURE(spacePos != TString::npos, "bad line '" << line << "' in lister");
                TUsingWideStringType form = StringToUtf32(line.substr(0, spacePos));
                form = StripStringRight(form);
                TVector<TString> grams;
                StringSplitter(line.substr(spacePos + 1)).Split(',').SkipEmpty().Collect(&grams);
                form = Norm(form);
                form = StripStringLeft(form, [](const auto* c) { return *c == '['; });
                SubstGlobal(form, TParseConstants::LeftBracket, TParseConstants::Tilde);
                SubstGlobal(form, TParseConstants::RightBracket, TParseConstants::Empty);
                forms.emplace_back(form, grams);
            }
        }
        if (!forms.empty() && !lemma.empty()) {
            result.emplace_back(lemma, forms);
        }
        return result;
    }

    //todo: use KMP
    template <class TSeq>
    bool HasSubstr(const TSeq& v, const TSeq& sequence) {
        for (size_t i = 0; i + sequence.size() <= v.size(); ++i) {
            bool ok = true;
            for (size_t j = 0; j < sequence.size(); ++j) {
                if (sequence[j] != v[i + j]) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                return true;
            }
        }
        return false;
    }

    template <class TSeq>
    TSeq GetLongestCommonSubstr(const TVector<TSeq>& data) {
        TSeq result;
        if (data.size() > 1 && !data[0].empty()) {
            for (size_t i = 0; i < data[0].size(); ++i) {
                for (size_t j = 0; j < data[0].size() + 1 - i; ++j) {
                    if (j > result.size()) {
                        bool ok = true;
                        TSeq candidate(data[0].begin() + i, data[0].begin() + i + j);
                        for (const auto& item : data) {
                            if (!HasSubstr(item, candidate)) {
                                ok = false;
                                break;
                            }
                        }
                        if (ok) {
                            result = candidate;
                        }
                    }
                }
            }
        }
        return result;
    }

    template <class TSeq>
    TSeq GetLongestCommonPrefix(const TVector<TSeq>& data) {
        if (data.empty()) {
            return TSeq();
        }
        TSeq result;
        for (size_t i = 0; i < data[0].size(); ++i) {
            for (const auto& item : data) {
                if (i >= item.size() || item[i] != data[0][i]) {
                    return result;
                }
            }
            result.push_back(data[0][i]);
        }
        return result;
    }

    /*TUsingWideStringType WordToUtf32(TString word, bool shift) {
        if (shift) {
            size_t tilde = word.find("~");
            if (tilde != TString::npos) {
                word = word.substr(tilde + 1);
            }
        }
        return StringToUtf32(word);
    }*/
    /*TVector<TUsingWideStringType> ConvertWordsToUtf32(const TVector<TString>& words, bool shift=true) {
        TVector<TUsingWideStringType> wordsUtf32;
        wordsUtf32.reserve(words.size());
        for (const TString& word : words) {
            wordsUtf32.push_back(WordToUtf32(word, shift));
        }
        return wordsUtf32;
    }*/
    TUsingWideStringType ShiftWord(TUsingWideStringType word) {
        size_t tilde = word.find(TParseConstants::Tilde);
        if (tilde != TUsingWideStringType::npos) {
            word = word.substr(tilde + 1);
        }
        return word;
    }

    class IStemmer {
    protected:
        virtual TUsingWideStringType GetStem(const TVector<TUsingWideStringType>& words) const = 0;

    public:
        struct TStemmatization {
            TUsingWideStringType Stem;
            TUsingWideStringType LexEnd;
        };
        TStemmatization operator()(const TVector<TUsingWideStringType>& forms, const TUsingWideStringType& lemma) const {
            auto wordsUtf32 = forms;
            wordsUtf32.push_back(lemma);
            Y_VERIFY(lemma.empty() || !wordsUtf32.back().empty());
            TStemmatization result;
            result.Stem = GetStem(wordsUtf32);
            result.LexEnd = wordsUtf32.back().substr(result.Stem.size(), wordsUtf32.back().size());

            return result;
        }
        virtual ~IStemmer() {
        }
    };

    class TLongestCommonSubstrStemmer: public IStemmer {
    protected:
        TUsingWideStringType GetStem(const TVector<TUsingWideStringType>& words) const override {
            return GetLongestCommonSubstr(words);
        }
    };

    class TLongestCommonPrefixStemmer: public IStemmer {
    protected:
        TUsingWideStringType GetStem(const TVector<TUsingWideStringType>& words) const override {
            TVector<TUsingWideStringType> shiftedWords;
            for (const auto& word : words) {
                shiftedWords.emplace_back(ShiftWord(word));
            }
            return GetLongestCommonPrefix(shiftedWords);
        }
    };

    class IGrammMaker {
    public:
        virtual TVector<TString> operator()(const TVector<TVector<TString>>& gramms) const = 0;
        virtual ~IGrammMaker() {
        }
    };

    class TLongestCommonPrefixGrammMaker: public IGrammMaker {
    public:
        TVector<TString> operator()(const TVector<TVector<TString>>& gramms) const override {
            return GetLongestCommonPrefix(gramms);
        }
    };

    class TCommonSubsetGrammMaker: public IGrammMaker {
    public:
        TVector<TString> operator()(const TVector<TVector<TString>>& words) const override {
            if (words.empty()) {
                return TVector<TString>();
            }
            TVector<TString> common(words[0].begin(), words[0].end());
            Sort(common.begin(), common.end());
            for (size_t i = 1; i < words.size(); ++i) {
                TVector<TString> word = words[i];
                Sort(word.begin(), word.end());
                TVector<TString> nextCommon;
                nextCommon.reserve(common.size());
                SetIntersection(common.begin(), common.end(), word.begin(), word.end(), std::insert_iterator<TVector<TString>>(nextCommon, nextCommon.begin()));
                common = nextCommon;
            }
            return common;
        }
    };

    TUsingWideStringType GetEnd(const TUsingWideStringType& form, const TUsingWideStringType& stem) {
        size_t tilde = form.find('~');
        if (tilde != TUsingWideStringType::npos) {
            size_t shft = tilde + 1;
            return form.substr(0, shft) + form.substr(shft + stem.size());
        }
        size_t pos = form.find(stem);
        if (pos != TUsingWideStringType::npos && pos != 0) {
            return form.substr(0, pos) + TParseConstants::Tilde + form.substr(pos + stem.size());
        }
        return form.substr(stem.size());
    }

    struct TScheme {
        TUsingWideStringType Stem;
        TUsingWideStringType LexEnd;
        TVector<TString> LexGram;
        struct TEnd {
            TUsingWideStringType End;
            TVector<TString> Gram;
            TEnd(const TUsingWideStringType& end, const TVector<TString>& gram)
                : End(end)
                , Gram(gram)
            {
            }
            bool operator<(const TEnd& other) const {
                return End < other.End || (End == other.End && Gram < other.Gram);
            }
            bool operator==(const TEnd& other) const {
                return End == other.End && Gram == other.Gram;
            }
        };
        TVector<TEnd> Ends;
    };

    TScheme GetScheme(const TChunk& chunk, const IStemmer& stemmer, const IGrammMaker& grammMaker) {
        TVector<TUsingWideStringType> forms;
        TVector<TVector<TString>> gramms;
        for (const auto& form : chunk.Forms) {
            forms.push_back(form.Text);
            gramms.push_back(form.Grams);
        }
        TScheme result;
        {
            const auto& stemmatization = stemmer(forms, chunk.Lemma);
            result.Stem = stemmatization.Stem;
            result.LexEnd = stemmatization.LexEnd;
        }
        result.LexGram = grammMaker(gramms);
        TVector<TString> lexGram = result.LexGram;
        Sort(lexGram.begin(), lexGram.end());
        //Y_ASSERT(result.LexEnd.empty() || (result.LexEnd[0] & 0b11000000) != 0b10000000);
        for (const auto& form : chunk.Forms) {
            TUsingWideStringType end = GetEnd(form.Text, result.Stem);
            TVector<TString> curGram = form.Grams;
            Sort(curGram.begin(), curGram.end());
            TVector<TString> gram;
            gram.reserve(curGram.size());
            SetDifference(curGram.begin(), curGram.end(), lexGram.begin(), lexGram.end(), std::insert_iterator<TVector<TString>>(gram, gram.begin()));
            result.Ends.emplace_back(end, gram);
        }
        Sort(result.Ends.begin(), result.Ends.end());
        result.Ends.erase(Unique(result.Ends.begin(), result.Ends.end()), result.Ends.end());
        return result;
    }
}

namespace NDictBuild {
    TString FullSchemeToString(const TFullScheme& scheme) {
        TStringBuilder result;
        TString stemsOut = JoinRange(" ", scheme.Stems.begin(), scheme.Stems.end());
        TMap<TString, TVector<size_t>> endsMap;
        TString endsOut = EndsMapToString(scheme.EndsMap);
        result
            << "0\t"
            << scheme.LexEnd
            << '='
            << scheme.LexGram
            << '\t'
            << endsOut
            << '\t'
            << stemsOut;
        return result;
    }

    void MakeSchemes(IInputStream& in, IOutputStream& schemesOutStream, IOutputStream& grammarOutStream, const TDictBuildParams& params, TSet<TUsingWideStringType>* allForms) {
        auto chunks = ListerChunk(in);
        if (allForms) {
            for (const auto& chunk : chunks) {
                for (const auto& form : chunk.Forms) {
                    allForms->insert(form.Text);
                }
            }
        }
        THolder<IStemmer> stemmer;
        if (params.SchemesLcs) {
            stemmer = MakeHolder<TLongestCommonSubstrStemmer>();
        } else {
            stemmer = MakeHolder<TLongestCommonPrefixStemmer>();
        }
        THolder<IGrammMaker> grammMaker;
        if (params.SchemesAsList) {
            grammMaker = MakeHolder<TLongestCommonPrefixGrammMaker>();
        } else {
            grammMaker = MakeHolder<TCommonSubsetGrammMaker>();
        }
        TMap<TVector<TString>, size_t> grammar;
        TMap<TNumberedScheme, TSet<TUsingWideStringType>> schemes;
        for (auto chunk : chunks) {
            bool hasSpace = chunk.Lemma.find(' ') != TUsingWideStringType::npos;
            for (const auto& form : chunk.Forms) {
                if (form.Text.find(' ') != TUsingWideStringType::npos) {
                    hasSpace = true;
                    break;
                }
                if (hasSpace) {
                    continue;
                }
            }
            TScheme scheme = GetScheme(chunk, *stemmer, *grammMaker);
            scheme.Stem = TParseConstants::Underscore + scheme.Stem;
            TNumberedScheme parsedScheme;
            parsedScheme.LexEnd = scheme.LexEnd;
            for (const auto& end : scheme.Ends) {
                if (!grammar.contains(end.Gram)) {
                    grammar[end.Gram] = grammar.size() + 1;
                }
                parsedScheme.Ends.emplace_back(end.End, grammar[end.Gram]);
            }
            if (!grammar.contains(scheme.LexGram)) {
                grammar[scheme.LexGram] = grammar.size() + 1;
            }
            parsedScheme.LexGram = grammar[scheme.LexGram];
            schemes[parsedScheme].insert(scheme.Stem);
        }
        TVector<TFullScheme> allSchemes(schemes.begin(), schemes.end());
        //schemes with most number of stems first
        Sort(allSchemes.begin(), allSchemes.end());
        for (const auto& scheme : allSchemes) {
            scheme.Save(&schemesOutStream);
            //schemesOutStream << FullSchemeToString(scheme) << Endl;
        }

        size_t gramNum = 0;
        TVector<TNumberedGrammar> allGrammar(grammar.begin(), grammar.end());
        Sort(allGrammar.begin(), allGrammar.end(), [](const TNumberedGrammar& a, const TNumberedGrammar& b) {
            return a.second < b.second;
        });
        for (const auto& gram : allGrammar) {
            grammarOutStream
                << (++gramNum)
                << '\t'
                << JoinRange(",", gram.first.begin(), gram.first.end())
                << Endl;
        }
    }
}
