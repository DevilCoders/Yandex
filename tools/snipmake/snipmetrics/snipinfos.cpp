#include "snipinfos.h"
#include <tools/snipmake/common/nohtml.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/read_helper/read_helper.h>
#include <kernel/snippets/wordstat/wordstat.h>
#include <kernel/snippets/wordstat/wordstat_data.h>
#include <contrib/libs/re2/re2/re2.h>
#include <library/cpp/charset/wide.h>
#include <kernel/lemmer/core/language.h>

#include <library/cpp/charset/ci_string.h>
#include <util/charset/unidata.h>
#include <util/string/strip.h>

namespace NSnippets {

const TVector<TString> ByLinkPrefixes = {
        "Ссылки на страницу содержат",
        "Спасылкі на старонку змяшчаюць",
        "Links to the page contain",
        "Беттің сілтемелер реті",
        "Şunu içeren sayfaya bağlantı",
        "Сәхифәгә алып барган сылтамалар эченә ала",
        "Посилання на сторінку містять"
};

bool HasUrlLike(const char* text, int length) {
    static RE2 re("(http(s)?://|www\\.|\\.ru|\\.ua|\\.kz|\\.com|\\.net|\\.by|\\.org|\\.htm|\\.php|\\.aspx|\\.рф)");
    re2::StringPiece src(text, length);
    return re.Match(src, 0, length, RE2::UNANCHORED, nullptr, 0);
}

int GetFragmetsCount(const char* text, int length) {
    static const RE2 re("((\\s*(((\\.){3,}|(…)+|\\n+)+)\\s*)+)");
    re2::StringPiece src(text, length);
    re2::StringPiece sp;
    int res = 0, b = 0, e = 0;
    while (RE2::FindAndConsume(&src, re, &sp)) {
        b = sp.data() - text;
        if (b != e)
            ++res;
        e = b + sp.size();
    }
    return (e != length) ? ++res : res;
}

int GetLinksCount(const TUtf16String& text) {
    static const RE2 re("(<l>.*?</l>)");
    TString utf8 = WideToUTF8(text);
    re2::StringPiece src(utf8.data(), utf8.size());
    re2::StringPiece sp;
    int res = 0;
    while (RE2::FindAndConsume(&src, re, &sp))
        ++res;
    return res;
}

TUtf16String ExtractBoldedText(const TUtf16String& text) {
    static const RE2 re("((<b>.*?</b>)|(<strong>.*?</strong>))");
    TString utf8 = WideToUTF8(text);
    re2::StringPiece src(utf8.data(), utf8.size());
    re2::StringPiece sp;
    TString res;
    while (RE2::FindAndConsume(&src, re, &sp))
        res += sp.ToString();
    return UTF8ToWide(res);
}

TSnipExtInfo::TSnipExtInfo(const TQueryy& query, const TReqSnip& reqSnip, const TConfig& cfg)
{
    TUtf16String title = NoHtml(StripString(reqSnip.TitleText));

    const TUtf16String delim = u"...";
    TUtf16String snip = reqSnip.SnipText[0].Text;
    for (size_t i = 1; i < reqSnip.SnipText.size(); ++i) {
        snip.append(delim);
        snip.append(reqSnip.SnipText[i].Text);
    }
    snip = NoHtml(StripString(snip));

    const int TITLE_ARC_SENT_ID = 0;
    const int SNIP_ARC_SENT_ID = 1;
    Storage.Reset(new TArchiveStorage);
    TArchiveView view;
    view.PushBack(&*Storage->Add(ARC_MISC, TITLE_ARC_SENT_ID, title, 0));
    view.PushBack(&*Storage->Add(ARC_MISC, SNIP_ARC_SENT_ID, snip, 0));
    SentsInfo.Reset(new TSentsInfo(nullptr, view, nullptr, false, false));
    Match.Reset(new TSentsMatchInfo(*SentsInfo, query, cfg));
    FirstSnipWordId = SentsInfo->WordCount();
    SnipTextOffset = SentsInfo->Text.size();
    for (int i = 0; i < SentsInfo->SentencesCount(); ++i) {
        if (SentsInfo->SentVal[i].ArchiveSent->SentId == SNIP_ARC_SENT_ID) {
            FirstSnipWordId = SentsInfo->FirstWordIdInSent(i);
            SnipTextOffset = SentsInfo->GetSentBuf(i).data() - SentsInfo->Text.data();
            break;
        }
    }
    for (const auto& byLinkPref : ByLinkPrefixes) {
        if (snip.StartsWith(UTF8ToWide(byLinkPref))) {
            IsByLink = true;
            break;
        }
    }
}

TColoringInfo::TColoringInfo(const TSnipExtInfo& snipExtInfo, const TReqSnip& reqSnip)
    : TitleBoldedRate(0)
    , SnippetBoldedRate(0)
    , LinksCount(GetLinksCount(reqSnip.TitleText))
{
    TUtf16String text = ExtractBoldedText(reqSnip.TitleText);
    size_t titleLen = snipExtInfo.SnipTextOffset;
    if (titleLen)
        TitleBoldedRate = (double) NoHtml(text).length() / titleLen;

    for (size_t i = 0; i < reqSnip.SnipText.size(); ++i) {
        text.append(ExtractBoldedText(reqSnip.SnipText[i].Text));
        LinksCount += GetLinksCount(reqSnip.SnipText[i].Text);
    }
    size_t snipLen = snipExtInfo.SentsInfo->Text.length();
    if (snipLen)
        SnippetBoldedRate = (double) NoHtml(text).length() / snipLen;
}

TSitelinkStatInfo::TSitelinkStatInfo(const TSitelink& sitelink, const TQueryy& query, const TConfig& cfg)
    : ColoredWordsRate(0)
    , DigitsRate(0)
    , UppercaseLettersRate(0)
    , IsUnknownLanguage(false)
    , IsDifferentLanguage(false)
    , HasAlpha(false)
{
    const TUtf16String& name = sitelink.Title;
    const TUtf16String txt = NoHtml(StripString(name));
    TArchiveStorage tempStorage;
    TArchiveView vName;
    vName.PushBack(&*tempStorage.Add(ARC_MISC, 0, txt, 0));
    const TSentsInfo sentsInfo(nullptr, vName, nullptr, true, false);
    const TSentsMatchInfo matchInfo(sentsInfo, query, cfg);

    WordsCount = matchInfo.WordsCount();
    SymbolsCount = txt.size();

    for (int i = 0; i < matchInfo.WordsCount(); ++i) {
        if (matchInfo.IsMatch(i)) {
            ++ColoredWordsRate;
        }
    }

    for (const TChar& c : txt) {
        if (IsDigit(c))
            ++DigitsRate;
        if (IsAlpha(c)) {
            HasAlpha = true;
            if (ToLower(c) != c)
                ++UppercaseLettersRate;
        }
    }

    if (HasAlpha) {
        TLangMask mask;
        for (int i = 0; i < matchInfo.WordsCount(); ++i) {
            mask |= matchInfo.GetWordLangs(i);
        }
        if (mask.Empty()) {
            IsUnknownLanguage = true;
        } else {
            if (!query.UserLangMask.Empty())
                IsDifferentLanguage = !mask.HasAny(query.UserLangMask);
        }
    }

    if (WordsCount)
        ColoredWordsRate /= WordsCount;
    if (SymbolsCount) {
        DigitsRate /= SymbolsCount;
        UppercaseLettersRate /= SymbolsCount;
    }
}

TSnipStatInfo::TSnipStatInfo(const TSnipExtInfo& snipExtInfo)
    : SnipLenInSents(snipExtInfo.SentsInfo->SentencesCount())
    , SnipLenInWords(snipExtInfo.Match->WordsCount())
    , SnipLenInBytes(snipExtInfo.SentsInfo->Text.length())
    , Less4WordsFragmentsCount(0)
    , TitleWordsInSnipPercent(0)
{
    THashSet<TUtf16String> titleWords;
    THashSet<TUtf16String> titleWordsInSnip;
    for (int wordId = 0; wordId < snipExtInfo.Match->WordsCount(); ++wordId) {
        TUtf16String word(snipExtInfo.SentsInfo->GetWordBuf(wordId));
        word.to_lower();
        if (wordId < snipExtInfo.FirstSnipWordId) { // title
            titleWords.insert(word);
        } else { // snip
            if (titleWords.contains(word)) {
                titleWordsInSnip.insert(word);
            }
        }
    }
    for (int sentId = 0; sentId < snipExtInfo.SentsInfo->SentencesCount(); ++sentId) {
        if (snipExtInfo.SentsInfo->GetSentLengthInWords(sentId) < 4) {
            ++Less4WordsFragmentsCount;
        }
    }
    TitleWordsInSnipPercent = (titleWords.size() > 0) ? (double) titleWordsInSnip.size() / titleWords.size() : 1;
}

TQueryWordsInSnipInfo::TQueryWordsInSnipInfo(const TSnipExtInfo& snipExtInfo)
    : MaxChainLen(0)
    , MatchesCount(0)
    , QueryWordsCount(snipExtInfo.Match->Query.UserPosCount())
    , NonstopQueryWordsCount(snipExtInfo.Match->Query.NonstopUserPosCount())
    , QueryWordsFoundInTitlePercent(0)
    , FirstMatchPos(-1)
    , HasQueryWordsInTitle(false)
    , NonstopUserWordCount(0)
    , NonstopLikeUserLemmaCount(0)
    , NonstopCleanUserLemmaCount(0)
    , NonstopUserWizCount(0)
    , NonstopUserExtenCount(0)
    , NonstopUserWordRate(0)
    , NonstopUserLemmaRate(0)
    , NonstopUserExtenRate(0)
    , UniqueWordRate(0)
    , UniqueLemmaRate(0)
    , UniqueGroupRate(0)
    , NonstopUserWordDensity(0)
    , NonstopUserLemmaDensity(0)
    , NonstopUserCleanLemmaDensity(0)
    , NonstopUserExtenDensity(0)
    , NonstopUserSynonymsCount(0)
{
    const int wordsCount = snipExtInfo.Match->WordsCount();

    int matchLen = 0;

    typedef THashSet<TUtf16String> TWSet;
    typedef TVector<TWSet> TLPack;
    TWSet uniqueWords;
    TLPack selectedLemmas;
    TWLemmaArray wordLemmas;
    for (int wordId = 0; wordId < wordsCount; ++wordId) {
            TUtf16String word(snipExtInfo.SentsInfo->GetWordBuf(wordId));
            word.to_lower();
            uniqueWords.insert(word);
            if (NLemmer::AnalyzeWord(word.data(), word.size(), wordLemmas, snipExtInfo.Match->Query.LangMask) ) {
                selectedLemmas.push_back(TWSet());
                TWSet& lemmas = selectedLemmas.back();
                for (size_t l = 0; l < wordLemmas.size(); ++l)
                    lemmas.insert(wordLemmas[l].GetText());
            }
            if (snipExtInfo.Match->IsStopword(wordId))
                continue;

            if (snipExtInfo.Match->IsMatch(wordId)) {
                if (MaxChainLen < ++matchLen)
                    MaxChainLen = matchLen;
                ++MatchesCount;

                if (FirstMatchPos < 0)
                    FirstMatchPos = wordId + 1;
            } else
                matchLen = 0;
    }

    TWordStat wordStat(snipExtInfo.Match->Query, *snipExtInfo.Match);
    if (snipExtInfo.FirstSnipWordId > 0) {
        wordStat.SetSpan(0, snipExtInfo.FirstSnipWordId - 1); // title sents words statistics
        QueryWordsFoundInTitlePercent = wordStat.Data().LikeWordSeenCount.NonstopUser;
    }
    if (snipExtInfo.FirstSnipWordId < wordsCount) {
        if (wordStat.GetSpansCount())   // add main snippet text
            wordStat.AddSpan(snipExtInfo.FirstSnipWordId, wordsCount - 1);
        else
            wordStat.SetSpan(snipExtInfo.FirstSnipWordId, wordsCount - 1);
    }

    const TWordStatData& data = wordStat.Data();
    NonstopUserWordCount        = data.WordSeenCount.NonstopUser;
    NonstopLikeUserLemmaCount   = data.LikeWordSeenCount.NonstopUser;
    NonstopUserWizCount         = data.LikeWordSeenCount.NonstopWizard;
    NonstopCleanUserLemmaCount  = Max(NonstopLikeUserLemmaCount - NonstopUserWordCount, 0);
    NonstopUserExtenCount       = NonstopLikeUserLemmaCount + NonstopUserWizCount;
    NonstopUserSynonymsCount    = data.SynWordSeenCount;

    size_t lemmCount = selectedLemmas.size();
    TVector<bool> seen(lemmCount, false);
    for (size_t k = 0; k < lemmCount; ++k)
        if (seen[k] == false) {
            UniqueGroupRate += 1;
            seen[k] = true;
            TVector<size_t> collide(1, k);
            for (size_t i = 0; i < collide.size(); ++i) {
                for (size_t j = 0; j < lemmCount; ++j)
                    if (seen[j] == false) {
                        const TWSet& i_lem = selectedLemmas[collide[i]];
                        const TWSet& j_lem = selectedLemmas[j];
                        for (TWSet::const_iterator l = i_lem.begin(), le = i_lem.end(); l != le; ++l)
                            if (j_lem.contains(*l)) {
                                seen[j] = true;
                                collide.push_back(j);
                                break;
                            }
                    }
                if (collide.size() == 1)
                    UniqueLemmaRate += 1;
            }
        }

    if (wordsCount) {
        UniqueWordRate  = (double) uniqueWords.size() / wordsCount;
        UniqueLemmaRate /= wordsCount;
        UniqueGroupRate /= wordsCount;

        NonstopUserWordDensity = (double) NonstopUserWordCount / wordsCount;
        NonstopUserLemmaDensity = (double) NonstopLikeUserLemmaCount / wordsCount;
        NonstopUserCleanLemmaDensity = (double) NonstopCleanUserLemmaCount / wordsCount;
        NonstopUserWizDensity = (double) NonstopUserWizCount / wordsCount;
        NonstopUserExtenDensity = (double) NonstopUserExtenCount / wordsCount;
    }

    if (NonstopQueryWordsCount) {
        NonstopUserWordRate = NonstopUserWordCount / NonstopQueryWordsCount;
        NonstopUserLemmaRate = NonstopLikeUserLemmaCount / NonstopQueryWordsCount;
        NonstopUserExtenRate = NonstopUserExtenCount / (NonstopQueryWordsCount+ snipExtInfo.Match->Query.NonstopWizardPosCount());
        QueryWordsFoundInTitlePercent /= NonstopQueryWordsCount;
    }
}

TSnipReadabilityInfo::TSnipReadabilityInfo(const TSnipExtInfo& snipExtInfo, const TPornoTermWeight& pornoWeighter, TSymbolsStat& Inspector)
    : NotReadableCharsRate(0)
    , DiffLanguageWordsRate(0)
    , UppercaseLettersRate(0)
    , DigitsRate(0)
    , TrashWordsRate(0)
    , AverageWordLength(0)
    , FragmentsCount(0)
    , HasUrl(false)
    , HasMenuLike(false)
    , HasPornoWords(false)
    , CharsStat(Inspector)
{
    TString utf8Snip = WideToUTF8(TWtringBuf(snipExtInfo.SentsInfo->Text).Skip(snipExtInfo.SnipTextOffset));
    FragmentsCount = GetFragmetsCount(utf8Snip.data(), utf8Snip.size());
    TString utf8Text = WideToUTF8(snipExtInfo.SentsInfo->Text);
    HasUrl = HasUrlLike(utf8Text.data(), utf8Text.size());

    size_t snipLenInChars = 0;
    size_t menuLikeConsecutiveWordsCount = 0;

    for (int wordId = 0; wordId < snipExtInfo.Match->WordsCount(); ++wordId) {
        TLangMask wordLangs = snipExtInfo.Match->GetWordLangs(wordId);
        if (!wordLangs.none() && !wordLangs.HasAny(snipExtInfo.Match->Query.UserLangMask)) {
            ++DiffLanguageWordsRate;
        }
        TWtringBuf word = snipExtInfo.SentsInfo->GetWordBuf(wordId);
        if (!HasPornoWords && pornoWeighter.GetTermWeight(TCiString(WideToChar(word, CODES_YANDEX)))) {
            HasPornoWords = true;
        }
        snipLenInChars += word.size();
        for (TChar c : word) {
            CharsStat.Test(c);
        }
        for (TChar c : word) {
            if (IsAlpha(c) && ToUpper(c) == c) {
                ++UppercaseLettersRate;
            }
        }
        if (!HasMenuLike && wordId >= snipExtInfo.FirstSnipWordId) {
            bool menuLikeWord = false;
            bool wasSpaceGap = wordId == 0 || IsSpace(*(word.data() - 1));
            if (!snipExtInfo.Match->IsMatch(wordId) && wasSpaceGap) {
                TWtringBuf str = word;
                while (str && IsDigit(str[0])) {
                    str.Skip(1);
                }
                if (str && ToUpper(str[0]) == str[0]) {
                    menuLikeWord = true;
                }
            }
            if (menuLikeWord) {
                menuLikeConsecutiveWordsCount++;
                if (menuLikeConsecutiveWordsCount == 3) {
                    HasMenuLike = true;
                }
            } else {
                menuLikeConsecutiveWordsCount = 0;
            }
        }
    }

    if (snipLenInChars > 0) {
        int end = snipExtInfo.Match->WordsCount() - 1;
        TReadabilityHelper trash(false);
        trash.AddRange(*snipExtInfo.Match.Get(), 0, end);
        TrashWordsRate = trash.CalcReadability();
        DigitsRate = trash.Digit / (double) snipLenInChars;
        NotReadableCharsRate = (snipExtInfo.Match->StrangeGapsInRange(0, end) + snipExtInfo.Match->TrashInGapsInRange(0, end)) / (double) snipLenInChars;
        UppercaseLettersRate /= (double) snipLenInChars;
        AverageWordLength = snipLenInChars / (end + 1.0);
        DiffLanguageWordsRate /= (end + 1.0);
    }
}

};
