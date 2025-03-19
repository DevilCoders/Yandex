#include "normalize.h"
#include <kernel/normalize_by_lemmas/special_words.pb.h>
#include <kernel/gazetteer/gztarticle.h>
#include <kernel/gazetteer/richtree/gztres.h>
#include <kernel/gazetteer/simpletext/simpletext.h>
#include <kernel/remorph/input/wtroka_input_symbol.h>
#include <kernel/remorph/matcher/remorph_matcher.h>

#include <kernel/lemmer/core/wordinstance.h>
#include <library/cpp/token/charfilter.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/string/type.h>
#include <util/string/vector.h>
#include <util/string/subst.h>
#include <util/generic/maybe.h>
#include <util/string/split.h>

TNormalizeByLemmasInfo::TNormalizeByLemmasInfo(const TString& specialWordsGztFileName,
                                               const TString& regexpFileName)
    : SpecialWordsGzt(new TGazetteer(specialWordsGztFileName))
    , MatcherPtr(regexpFileName ? NReMorph::TMatcher::Parse(TFileInput(regexpFileName).ReadAll()) : nullptr)
{
}

TString KeepDigitsOnly(TStringBuf src) {
    TString result;
    result.reserve(src.size());
    for (char c : src) {
        if (IsDigit(c)) {
            result.push_back(c);
        }
    }
    return result;
}

using PhoneHandler = std::function<bool(const TStringBuf&, size_t, size_t)>;

void RunPhoneSearch(const TStringBuf& query, PhoneHandler russianHandler, PhoneHandler foreignHandler) {
    auto isSupportingCharacter = [=](const char ch) {
        return EqualToOneOf(ch , '(', ')', '-', ' ');
    };

    constexpr size_t fullNumberLen = 11;
    constexpr size_t invalidIndex = std::numeric_limits<size_t>::max();

    ui32 digitCount = 0;
    size_t russianPhoneStart = invalidIndex;
    char chPrev = 0;
    bool phoneHasSupportingChars = false;
    for (size_t c = 0; c < query.size(); ++c) {
        const char ch = query.at(c);
        if (IsDigit(ch)) {
            ++digitCount;
            if (digitCount == fullNumberLen) {
                if (russianPhoneStart != invalidIndex) {
                    if (!russianHandler(query, russianPhoneStart, c)) {
                        break;
                    }
                    russianPhoneStart = invalidIndex;
                } else if (!phoneHasSupportingChars) {
                    if (!foreignHandler(query, c - digitCount + 1, c)) {
                        break;
                    }
                }
                digitCount = 0;
            }
            if (digitCount == 1) {
                if (ch == '7') {
                    russianPhoneStart = chPrev == '+' ? c - 1 : c;
                } else if (ch == '8' && chPrev != '+') {
                    russianPhoneStart = c;
                }
                phoneHasSupportingChars = false;
            }
        } else if (isSupportingCharacter(ch)) {
            phoneHasSupportingChars = true;
        } else {
            digitCount = 0;
            russianPhoneStart = invalidIndex;
        }
        chPrev = ch;
    }
}

bool DoesContainPhone(const TStringBuf& query) {
    bool russianPhoneFound = false;

    auto russianHandler = [&russianPhoneFound](const TStringBuf& /*query*/, size_t /*from*/, size_t /*to*/) -> bool {
        russianPhoneFound = true;
        return false;
    };

    auto foreignHandler = [](const TStringBuf& /*query*/, size_t /*from*/, size_t /*to*/) -> bool {
        return true;
    };

    RunPhoneSearch(query, russianHandler, foreignHandler);
    return russianPhoneFound;
}

TString NormalizePhones(const TStringBuf& query) {
    struct TMatch {
        TMaybe<size_t> Start;
        TMaybe<size_t> End;
        TString Substitute;

        bool IsComplete() {
            return Start.Defined() && End.Defined();
        }

        void Reset() {
            Start.Clear();
            End.Clear();
        }
    };

    TVector<TMatch> matches;

    auto russianHandler = [&matches](const TStringBuf& query, size_t from, size_t to) -> bool {
        TMatch russianPhoneMatch{from, to, KeepDigitsOnly(query.substr(from, to - from + 1))};
        matches.push_back(russianPhoneMatch);
        return true;
    };

    auto foreignHandler = [&matches](const TStringBuf& query, size_t /*from*/, size_t to) -> bool {
        TMatch foreignPhoneSpoiler{to, to, TString(" ") + query[to]};
        // FACTS-2285: spoil foreign phones by inserting a space before the last digit
        matches.push_back(foreignPhoneSpoiler);
        return true;
    };

    RunPhoneSearch(query, russianHandler, foreignHandler);

    if (matches.empty()) {
        return TString(query);
    }

    TString norm;
    norm.reserve(query.size() + matches.size());
    size_t nextStartPos = 0;
    for (size_t i = 0; i < matches.size(); ++i) {
        norm += query.substr(nextStartPos, *matches[i].Start - nextStartPos);
        norm += matches[i].Substitute;
        nextStartPos = *matches[i].End + 1;
    }
    norm += query.substr(nextStartPos, query.size() - nextStartPos);
    return norm;
}

TUtf16String NormalizePhones(const TUtf16String& queryWide) {
    TString query = WideToUTF8(queryWide);
    return UTF8ToWide(NormalizePhones(query));
}

TUtf16String NormalizeByLemmas(const TUtf16String& req, const TNormalizeByLemmasInfo& info, NReMorph::TMatchResults* matchResults) {
    TUtf16String res = NormalizePhones(req);
    return NormalizeByLemmas(res, info.SpecialWordsGzt, info.MatcherPtr, matchResults);
}

namespace {
    class TSplitHandler : public ITokenHandler {
        TVector<TUtf16String>* Res;
    public:
        inline TSplitHandler(TVector<TUtf16String>* res)
            : Res(res)
        {}

        ~TSplitHandler() override {}

        void AddToken(const TUtf16String& token) {
            Res->push_back(NormalizeUnicode(token));
        }

        void SplitByDotAndAddTokens(const TUtf16String& token) {
            static const TUtf16String wdot = u".";
            TVector<TUtf16String> subtokens;
            StringSplitter(token).SplitByString(wdot.data()).SkipEmpty().Collect(&subtokens);
            for (const TUtf16String& subtoken : subtokens) {
                Res->push_back(NormalizeUnicode(subtoken));
            }

        }

        void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override {
            TUtf16String tok;

            switch (type) {
                case NLP_SENTBREAK:
                case NLP_PARABREAK:
                case NLP_MISCTEXT:  // special check for smileys
                    tok = TUtf16String(token.Token, token.Leng);
                    Strip(tok);
                    if (tok.size() > 1) {
                        AddToken(tok);
                    }
                    break;
                case NLP_WORD:
                case NLP_INTEGER:
                case NLP_FLOAT:
                case NLP_MARK:
                    if (token.SubTokens.size() < 2) {
                        tok = TUtf16String(token.Token, token.Leng);
                        if (type == NLP_FLOAT) {
                            SplitByDotAndAddTokens(tok);
                        } else {
                            AddToken(tok);
                        }
                    } else {
                        for (size_t i = 0; i < token.SubTokens.size(); ++i) {
                            size_t startPos = token.SubTokens[i].Pos;
                            size_t endPos = token.SubTokens[i].EndPos();

                            tok = TUtf16String(token.Token + startPos, token.Token + endPos);
                            AddToken(tok);
                        }
                    }
                    break;
                case NLP_END:
                    break;
            }
        }
    };
} // end of anonymous namespace

static void SplitQuery(const TUtf16String& query, TVector<TUtf16String>* res) {
    TSplitHandler handler(res);
    TNlpTokenizer tokenizer(handler);
    tokenizer.Tokenize(query);
}

static void InitializeWordInstances(const TVector<TUtf16String>& splitted, TVector<TWordInstance>& words, TVector<const TWordInstance*>& wordsPtrs) {
    words.resize(splitted.size());
    wordsPtrs.resize(splitted.size());
    for (size_t i = 0; i < splitted.size(); ++i) {
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(splitted[i].data(), splitted[i].size(), lemmas, TLangMask(LANG_RUS, LANG_ENG));
        TVector<const TYandexLemma*> lemmaPtrs;
        for (const auto& lemma : lemmas) {
            lemmaPtrs.push_back(&lemma);
        }
        words[i].Init(lemmaPtrs, TLanguageContext(), fGeneral, false);
        wordsPtrs[i] = &words[i];
    }
}

static void AnalyzeWordsWithGazetteer(const TVector<const TWordInstance*>& wordsPtrs, const THolder<const TGazetteer>& specialWordsGazetteer,
        THashSet<size_t>& stopwordsIndices, THashSet<size_t>& markersIndices, TSet<TUtf16String>& foundMarkers, THashMap<size_t, TUtf16String>& replacements)
{
    THashSet<size_t> leftWordsIndices;
    NGzt::TArticleIter<TVector<const TWordInstance*>> matches;
    specialWordsGazetteer->IterArticles(wordsPtrs, &matches);
    for (; matches.Ok(); ++matches) {
        NGzt::TArticlePtr artPtr = specialWordsGazetteer->GetArticle(matches);
        const TSpecialWordProto* art = artPtr.As<TSpecialWordProto>();

        if (art != nullptr) {
            TSpecialWordProto::EType type = (TSpecialWordProto::EType)art->GetType();
            size_t pos = matches.GetWords().Pos();
            size_t size = matches.GetWords().Size();
            for (size_t i = 0; i < size; ++i) {
                if (type == TSpecialWordProto::STOPWORD) {
                    stopwordsIndices.insert(i + pos);
                } else if (type == TSpecialWordProto::STOPWORD_EXCEPTION) {
                    leftWordsIndices.insert(i + pos);
                } else if (type == TSpecialWordProto::MARKER) {
                    markersIndices.insert(i + pos);
                    foundMarkers.insert(UTF8ToWide(art->GetMarker()));
                } else if (type == TSpecialWordProto::REPLACED_WORD && !!art->GetReplacement()) {
                    replacements[i + pos] = UTF8ToWide(art->GetReplacement());
                }
            }
        }
    }
    for (const size_t& idx : leftWordsIndices) {
        stopwordsIndices.erase(idx);
    }

    if (markersIndices.size() == wordsPtrs.size()) {
        markersIndices.clear();
        foundMarkers.clear();
    }

    for (const size_t& idx : markersIndices) {
        stopwordsIndices.erase(idx);
    }
}

static void FindMatches(const TVector<TUtf16String>& splitted,
                        const NReMorph::TMatcherPtr matcherPtr,
                        NReMorph::TMatchResults& matchResults) {

    NReMorph::TWtrokaInputSymbols inputSymbols;
    for (size_t i = 0; i < splitted.size(); ++i) {
        inputSymbols.push_back(new NReMorph::TWtrokaInputSymbol(i, splitted[i]));
    }

    matcherPtr->MatchAll(inputSymbols, matchResults);
}

TUtf16String NormalizeByLemmas(const TUtf16String& req,
                               const THolder<const TGazetteer>& specialWordsGazetteer,
                               const NReMorph::TMatcherPtr matcherPtr,
                               NReMorph::TMatchResults* matchResults) {
    TVector<TUtf16String> splitted;
    SplitQuery(req, &splitted);

    if (matcherPtr != nullptr) {
        NReMorph::TMatchResults banMatchResults;
        FindMatches(splitted, matcherPtr, banMatchResults);
        for (const NReMorph::TMatchResultPtr& matchResultPtr : banMatchResults) {
            if (!!matchResultPtr) {
                if (matchResults) {
                    *matchResults = std::move(banMatchResults);
                }
                return req;
            }
        }
    }

    TVector<TWordInstance> words;
    TVector<const TWordInstance*> wordsPtrs;
    InitializeWordInstances(splitted, words, wordsPtrs);

    THashSet<size_t> stopwordsIndices;
    THashSet<size_t> markersIndices;
    TSet<TUtf16String> foundMarkers;
    THashMap<size_t, TUtf16String> replacements;
    AnalyzeWordsWithGazetteer(wordsPtrs, specialWordsGazetteer, stopwordsIndices, markersIndices, foundMarkers, replacements);
    const size_t leftWordsCount = splitted.size() - stopwordsIndices.size();
    const size_t markersCount = markersIndices.size();
    const bool justOneNonMarkerWord = (leftWordsCount <= markersCount + 1);

    TSet<TWtringBuf> leftWords;
    TVector<TWtringBuf> numbers;
    for (size_t i = 0; i < splitted.size(); ++i) {
        if (!markersIndices.contains(i) && (justOneNonMarkerWord || (!stopwordsIndices.contains(i)))) {
            const TUtf16String* replacement = replacements.FindPtr(i);
            const TWtringBuf word = replacement == nullptr
                ? splitted[i]
                : *replacement;
            if (IsNumber(word))
                numbers.push_back(word);
            else {
                if (!justOneNonMarkerWord && replacement == nullptr && words[i].NumLemmas() > 0) {
                    leftWords.insert(words[i].LemsBegin()->GetLemma());
                } else {
                    leftWords.insert(word);
                }
            }
        }
    }

    TVector<TWtringBuf> finalListOfWords;
    finalListOfWords.assign(leftWords.begin(), leftWords.end());
    finalListOfWords.insert(finalListOfWords.end(), numbers.begin(), numbers.end());
    static const TUtf16String ws = u" ";
    for (const TUtf16String& marker : foundMarkers) {
        finalListOfWords.push_back(marker);
    }
    return JoinStrings(finalListOfWords.begin(), finalListOfWords.end(), ws);
}
