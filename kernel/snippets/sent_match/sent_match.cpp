#include "sent_match.h"

#include "similarity.h"
#include "is_definition.h"

#include <kernel/snippets/archive/zone_checker/zone_checker.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/data/pornodict.h>
#include <kernel/snippets/dynamic_data/answer_models.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/qtree/wordnormalizer.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_info/sentword.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/smartcut/pixel_length.h>
#include <kernel/snippets/telephone/telephones.h>

#include <kernel/tarc/iface/tarcface.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/stopwords/stopwords.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/generic/algorithm.h>
#include <util/charset/wide.h>
#include <utility>

namespace NSnippets
{
    struct TSentsMatchInfo::TData {
        struct TWordsMInfo {
            struct TSumWord {
                int SumMatch = 0;
                int SumExactMatch = 0;
                int SumShorts = 0;
                int SumStrangeGaps = 0;
                int SumTrashInGaps = 0;
                int SumPunctInGaps = 0;
                int SumSlashesInGap = 0;
                int SumSlashesInsideGap = 0;
                int SumVertsInGap = 0;
                int SumVertsInsideGap = 0;
                int SumPunctReadInGap = 0;
                int SumPunctReadInsideGap = 0;
                int SumPunctBalInGap = 0;
                int SumPunctBalInsideGap = 0;
                int SumTrashAsciiInGap = 0;
                int SumTrashAsciiInsideGap = 0;
                int SumTrashUnicodeInGap = 0;
                int SumTrashUnicodeInsideGap = 0;
                int SumCaps = 0;
                int SumFooterWords = 0;
                int SumContentWords = 0;
                int SumHeaderWords = 0;
                int SumMainContentWords = 0;
                double SumSegmentWeight = 0.0;
                int SumMainHeaderWords = 0;
                int SumMenuWords = 0;
                int SumReferatWords = 0;
                int SumAuxWords = 0;
                int SumLinksWords = 0;
                int SumKandziWords = 0;
                int SumLangMatchs = 0;
                int SumAlphas = 0;
                int SumCyrAlphas = 0;
                int SumDigits = 0;
                int SumTelephoneWordBegin = 0;
                int SumTelephoneWordEnd = 0;
                int SumDateWordBegin = 0;
                int SumDateWordEnd = 0;
                int SumRegionMatch = 0;
                int SumUnmatchedByLinkPornCount = 0;
                float SumIsAnswerWeight = 0.0;
            };
            TVector<TSumWord> WordAcc;
            struct TWordVal {
                bool MIsExactUserPhone = false;
                const TPositionsInQuery* ExactMatchedPositions = nullptr;
                const TPositionsInQuery* NotExactMatchedLemmaIds = nullptr;
                const TPositionsInQuery* SynMatchedLemmaIds = nullptr;
                const TPositionsInQuery* AlmostUserWordsMatchedLemmaIds = nullptr;
                const TPositionsInQuery* MatchedPositionsInRegionPhrase = nullptr;
                bool WordAfterTerminal = false;
                TUtf16String LowerWord;
                TLangMask WordLangs;
                bool IsStopword = false;
            };
            TVector<TWordVal> WordVal;

            mutable TVector<bool> TmpLemm;
            mutable TVector<bool> TmpPos;
            mutable TVector<ESubTGrammar> WordSubGrammar; // This field is computed lazy
            TVector<int> Nop;
        };
        TWordsMInfo WordsMInfo;
        struct TSentsMInfo {
            struct TSumSent {
                int SumHasMatches = 0;
                int SumNavlike = 0;
                int SumQuality = 0;
                int SumInfoBonusSents = 0;
                int SumInfoBonusMatchSents = 0;
                double SumAnchorPercent = 0.0;
                int SumAdsSents = 0;
                int SumHeaderSents = 0;
                int SumPollSents = 0;
                int SumMenuSents = 0;
                int SumFooterSents = 0;
                int SumTextAreaSents = 0;
            };
            TVector<TSumSent> SentAcc;
            struct TSentVal {
                int LongestChain = 0;
                bool LooksLikeDefinition = 0;
                bool RepeatsTitle = 0;
                THolder<TEQInfo> EQInfo;
            };
            TVector<TSentVal> SentVal;

            int FirstMatchSentId;
            int LastMatchSentId;
        };
        TSentsMInfo SentsMInfo;
        THolder<TPixelLengthCalculator> PixelLengthCalculator; // lazy field
    };

    inline bool HasTerminal(const TWtringBuf& str)
    {
        for (wchar16 c : str) {
            if (IsTerminal(c) || IsDash(c) || IsHyphen(c) || (IsPairedPunct(c) && !IsQuotation(c))) {
                return true;
            }
        }
        return false;
    }

    inline bool IsSpaceGap(const TWtringBuf& str)
    {
        if (!str)
            return true;
        return IsSpace(str);
    }

    inline bool IsStrangeGap(TWtringBuf str)
    {
        if (!str)
            return false;
        if (!IsSpace(str.back()))
            return true;
        while (str && !IsSpace(str[0])) {
            str.Skip(1);
        }
        while (str && IsSpace(str[0])) {
            str.Skip(1);
        }
        return !str.empty();
    }

    inline void ReadabilityTrash(const TWtringBuf& str, int& slash, int& vert, int& punct, int& punctBal,
        int& trashAscii, int& trashUTF)
    {
        for (wchar16 c : str) {
            if (IsPunct(c)) {
                ++punct;
                if (IsLeftPunct(c)) {
                    ++punctBal;
                } else if (IsRightPunct(c)){
                    --punctBal;
                }
            } else {
                if (!IsSpace(c) && !IsQuotation(c) && !IsPermittedEdgeChar(c)) {
                    if (static_cast<std::make_unsigned<wchar16>::type>(c) < 256) {
                        ++trashAscii;
                    } else {
                        ++trashUTF;
                    }
                }
            }
            if (c == '/' || c == '\\') {
                ++slash;
            } else if (c == '|') {
                ++vert;
            }
        }
    }

    inline int TrashInGap(const TWtringBuf& str)
    {
        int res = 0;
        for (wchar16 c : str) {
            switch (c) {
                case '<':
                case '>':
                case '\\':
                case '|':
                case '#':
                case '*':
                case '~':
                case '^':
                case '&':
                case '/':
                    ++res;
                    break;
            }
        }
        return res;
    }

    inline bool IsCapitalized(const TWtringBuf& word)
    {
        if (!word)
            return true;
        const wchar16 c = word[0];
        return ToUpper(c) == c && ToLower(c) != c;
    }

    inline void CountAlphaAndDig(const TWtringBuf& word, int& alpha, int& digit, int& cyrAlpha) {
        for (wchar16 c : word) {
            if (IsAlpha(c)) {
                ++alpha;
                if (c >= 0x400 && c <= 0x4FF) {
                    ++cyrAlpha;
                }
            } else {
                if (IsDigit(c)) {
                    ++digit;
                }
            }
        }
    }

    inline const TUtf16String& TryReplaceNumeric(const TUtf16String& token, NLP_TYPE type) {
        static const TUtf16String NUMERIC = u"__integer__";
        if (type == NLP_INTEGER) {
            return NUMERIC;
        }
        return token;
    }

    bool TSentsMatchInfo::LeaveStopword(int i, int pos) const
    {
        const THashSet<int>& pWanna = Query.Positions[pos].NeighborPositions;
        if (!pWanna)
            return false;
        const int d = 1;
        for (int j = i - d; j <= i + d; ++j) {
            if (j < 0 || j >= (int)SentsInfo.WordVal.size()) {
                continue;
            }
            if (j == i || !IsMatch(j)) {
                continue;
            }
            if (!IsExactMatch(i) && !IsExactMatch(j) && IsPureStopWordMatch(j)) {
                continue;
            }
            if (SentsInfo.WordId2SentId(i) != SentsInfo.WordId2SentId(j)) {
                continue;
            }
            for (int lemmaId : GetNotExactMatchedLemmaIds(j)) {
                for (int p : Query.Id2Poss[lemmaId]) {
                    if (pWanna.contains(p)) {
                        return true;
                    }
                }
            }
            for (int p : GetExactMatchedPositions(j)) {
                if (pWanna.contains(p)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool TSentsMatchInfo::LeaveStopword(int i) const
    {
        for (int lemmaId : GetNotExactMatchedLemmaIds(i)) {
            for (int pos : Query.Id2Poss[lemmaId]) {
                if (LeaveStopword(i, pos)) {
                    return true;
                }
            }
        }
        for (int pos : GetExactMatchedPositions(i)) {
            if (LeaveStopword(i, pos)) {
                return true;
            }
        }
        return false;
    }

    void TSentsMatchInfo::KillLonelyStopWords() {
        for (int i = 0; i < (int)SentsInfo.WordVal.size(); ++i) {
            if (IsPureStopWordMatch(i)) {
                if (!LeaveStopword(i)) {
                    Data->WordsMInfo.WordVal[i].NotExactMatchedLemmaIds = &Data->WordsMInfo.Nop;
                    Data->WordsMInfo.WordVal[i].ExactMatchedPositions = &Data->WordsMInfo.Nop;
                    Data->WordsMInfo.WordVal[i].AlmostUserWordsMatchedLemmaIds = &Data->WordsMInfo.Nop;
                }
            }
        }
    }

    void TSentsMatchInfo::FillShortWords() {
        int shortCnt = 0;
        const int shortThr = 6;

        for (int i = 0; i < SentsInfo.WordVal.ysize(); ++i) {
            if (SentsInfo.WordVal[i].Word.Len > 2) {
                shortCnt = 0;
            } else {
                ++shortCnt;
                if (shortCnt == shortThr) {
                    for (int j = 1; j < shortThr; ++j) {
                        Data->WordsMInfo.WordAcc[(i + 1) + j - shortThr].SumShorts += j;
                    }
                }
            }
            Data->WordsMInfo.WordAcc[i + 1].SumShorts = Data->WordsMInfo.WordAcc[i].SumShorts + (shortCnt >= shortThr);
        }
    }

    void TSentsMatchInfo::FillMatchSumms() {
        const TLangMask kandziMask(LANG_JPN, LANG_KOR, LANG_CHI);

        for (int i = 0; i < SentsInfo.WordVal.ysize(); ++i) {
            auto& wordAcc = Data->WordsMInfo.WordAcc[i + 1];
            const auto& wordAccPrev = Data->WordsMInfo.WordAcc[i];

            TLangMask wordLangs = Data->WordsMInfo.WordVal[i].WordLangs;
            const bool isExactMatch = IsExactMatch(i);
            const bool isMatch = IsMatch(i);
            const bool langMatched = DocLangId != LANG_UNK && !isMatch && (wordLangs.none() || wordLangs.SafeTest(DocLangId));
            const bool isKandzi = !isMatch && wordLangs.HasAny(kandziMask);

            wordAcc.SumExactMatch = wordAccPrev.SumExactMatch + isExactMatch;
            wordAcc.SumMatch = wordAccPrev.SumMatch + isMatch;
            wordAcc.SumLangMatchs = wordAccPrev.SumLangMatchs + langMatched;
            wordAcc.SumKandziWords = wordAccPrev.SumKandziWords + isKandzi;
        }
    }

    void TSentsMatchInfo::FillDates()
    {
        TVector<int> wordDateZoneId;
        //hint to mark last word, if it was the date
        wordDateZoneId.resize(SentsInfo.WordVal.size() + 1, 0);

        //create date zone spans checker
        TForwardInZoneChecker dateArchiveZoneChecker =
            TForwardInZoneChecker(SentsInfo.GetTextArcMarkup().GetZone(AZ_DATER_DATE).Spans);

        bool foundAny = false;
        for (size_t wordId = 0; wordId < SentsInfo.WordVal.size(); ++wordId) {
            const int wordSentId = SentsInfo.WordId2SentId(wordId);
            TWtringBuf origin = SentsInfo.WordVal[wordId].Origin;
            const TArchiveSent& arcSent = SentsInfo.GetArchiveSent(wordSentId);

            if (!origin || arcSent.SourceArc != ARC_TEXT) {
                continue;
            }

            const size_t wordOriginSentOffsetBeg = origin.data() - arcSent.Sent.data();
            //right bound inclusive
            const size_t wordOriginSentOffsetEnd = wordOriginSentOffsetBeg + origin.size() - 1;

            dateArchiveZoneChecker.SeekToWord(arcSent.SentId, wordOriginSentOffsetEnd);
            bool isDate = dateArchiveZoneChecker.IsWordInCurrentZone(arcSent.SentId, wordOriginSentOffsetBeg, wordOriginSentOffsetEnd);
            wordDateZoneId[wordId] = isDate ? dateArchiveZoneChecker.GetCurrentZoneId() : 0;
            foundAny = foundAny || isDate;
        }

        if (foundAny && wordDateZoneId.size() > 1) {
            //check if first date word is zone end
            Data->WordsMInfo.WordAcc[1].SumDateWordBegin = wordDateZoneId[0];
            Data->WordsMInfo.WordAcc[1].SumDateWordEnd = wordDateZoneId[1] != wordDateZoneId[0];

            for (size_t wordId = 1; wordId < wordDateZoneId.size() - 1; ++wordId) {
                const bool isZoneBegin = (wordDateZoneId[wordId] != wordDateZoneId[wordId - 1]);
                const bool isZoneEnd = (wordDateZoneId[wordId + 1] != wordDateZoneId[wordId]);
                const bool inZone = (wordDateZoneId[wordId] != 0);

                const int leftBoundsSum = Data->WordsMInfo.WordAcc[wordId].SumDateWordBegin + (isZoneBegin && inZone);
                const int rightBoundsSum = Data->WordsMInfo.WordAcc[wordId].SumDateWordEnd + (isZoneEnd && inZone);

                Data->WordsMInfo.WordAcc[wordId + 1].SumDateWordBegin = leftBoundsSum;
                Data->WordsMInfo.WordAcc[wordId + 1].SumDateWordEnd = rightBoundsSum;
            }
        }
    }

    void TSentsMatchInfo::FillTelephones()
    {
        TVector<int> wordTelephoneZoneId;
        //hint to mark last word, if it was the telephone
        wordTelephoneZoneId.resize(SentsInfo.WordVal.size() + 1, 0);

        //create telephone zone spans checker
        TForwardInZoneChecker telephoneArchiveZoneChecker =
            TForwardInZoneChecker(SentsInfo.GetTextArcMarkup().GetZone(AZ_TELEPHONE).Spans);

        const TArchiveZoneAttrs& telephoneArchiveAttrs = SentsInfo.GetTextArcMarkup().GetZoneAttrs(AZ_TELEPHONE);

        bool foundAny = false;
        for (size_t wordId = 0; wordId < SentsInfo.WordVal.size(); ++wordId) {
            const int wordSentId = SentsInfo.WordId2SentId(wordId);
            TWtringBuf origin = SentsInfo.WordVal[wordId].Origin;
            const TArchiveSent& arcSent = SentsInfo.GetArchiveSent(wordSentId);

            if (!origin || arcSent.SourceArc != ARC_TEXT) {
                continue;
            }

            const size_t wordOriginSentOffsetBeg = origin.data() - arcSent.Sent.data();
            //right bound inclusive
            const size_t wordOriginSentOffsetEnd = wordOriginSentOffsetBeg + origin.size() - 1;

            telephoneArchiveZoneChecker.SeekToWord(arcSent.SentId, wordOriginSentOffsetEnd);
            bool isTelephone = telephoneArchiveZoneChecker.IsWordInCurrentZone(arcSent.SentId, wordOriginSentOffsetBeg, wordOriginSentOffsetEnd);
            wordTelephoneZoneId[wordId] = isTelephone ? telephoneArchiveZoneChecker.GetCurrentZoneId() : 0;
            foundAny = foundAny || isTelephone;

            if (!telephoneArchiveZoneChecker.Empty() && isTelephone) {
                const TArchiveZoneSpan* currentArchiveSpan = telephoneArchiveZoneChecker.GetCurrentSpan();
                bool isSingleSentPhone = currentArchiveSpan->SentBeg == currentArchiveSpan->SentEnd;
                const THashMap<TString, TUtf16String>* attributes = telephoneArchiveAttrs.GetSpanAttrs(*currentArchiveSpan).AttrsHash;

                if (attributes && isSingleSentPhone) {
                    const TPhone userPhone = TUserTelephones::AttrsToPhone(*attributes);

                    // exact match
                    const auto* word = Query.LowerWords.FindPtr(ASCIIToWide(userPhone.ToPhoneWithCountry()));
                    if (!word) {
                        word = Query.LowerWords.FindPtr(ASCIIToWide(userPhone.ToPhoneWithArea()));
                    }
                    if (!word) {
                        word = Query.LowerWords.FindPtr(ASCIIToWide(userPhone.GetLocalPhone()));
                    }
                    if (word && Query.PositionsHasPhone(word->WordPositions)) {
                        Data->WordsMInfo.WordVal[wordId].ExactMatchedPositions = &word->WordPositions;
                        Data->WordsMInfo.WordVal[wordId].MIsExactUserPhone = true;
                    }

                    // non exact match
                    const auto* form = Query.LowerForms.FindPtr(ASCIIToWide(userPhone.GetLocalPhone()));
                    if (form && Query.IdsHasPhone(form->LemmaIds)) {
                        Data->WordsMInfo.WordVal[wordId].NotExactMatchedLemmaIds = &form->LemmaIds;
                    }
                }
            }
        }

        if (foundAny && wordTelephoneZoneId.size() > 1) {
            //check if first telephone word is zone end
            Data->WordsMInfo.WordAcc[1].SumTelephoneWordBegin = wordTelephoneZoneId[0];
            Data->WordsMInfo.WordAcc[1].SumTelephoneWordEnd = wordTelephoneZoneId[1] != wordTelephoneZoneId[0];

            for (size_t wordId = 1; wordId < wordTelephoneZoneId.size() - 1; ++wordId) {
                const bool isZoneBegin = (wordTelephoneZoneId[wordId] != wordTelephoneZoneId[wordId - 1]);
                const bool isZoneEnd = (wordTelephoneZoneId[wordId + 1] != wordTelephoneZoneId[wordId]);
                const bool inZone = (wordTelephoneZoneId[wordId] != 0);

                const int leftBoundsSum = Data->WordsMInfo.WordAcc[wordId].SumTelephoneWordBegin + (isZoneBegin && inZone);
                const int rightBoundsSum = Data->WordsMInfo.WordAcc[wordId].SumTelephoneWordEnd + (isZoneEnd && inZone);

                Data->WordsMInfo.WordAcc[wordId + 1].SumTelephoneWordBegin = leftBoundsSum;
                Data->WordsMInfo.WordAcc[wordId + 1].SumTelephoneWordEnd = rightBoundsSum;
            }
        }
    }

    void TSentsMatchInfo::InitWords()
    {
        Data->WordsMInfo.TmpLemm.resize(Query.IdsCount(), false);
        Data->WordsMInfo.TmpPos.resize(Query.PosCount(), false);
        Data->WordsMInfo.WordAcc.resize(SentsInfo.WordCount() + 1);
        Data->WordsMInfo.WordVal.resize(SentsInfo.WordCount());
        Data->WordsMInfo.WordSubGrammar.resize(SentsInfo.WordCount(), STG_UNKNOWN);

        const TPositionsInQuery* nop = &Data->WordsMInfo.Nop;

        TLangMask normLangs = Query.LangMask;
        normLangs.SafeSet(DocLangId);
        if (Query.RegionQuery.Get())
            normLangs |= Query.RegionQuery->LangMask;
        TWordNormalizer wordNormalizer(normLangs);
        TVector<TUtf16String> normalizedWords;
        TUtf16String lowerWord;

        for (int wid = 0; wid < SentsInfo.WordCount(); ++wid) {
            auto& wordVal = Data->WordsMInfo.WordVal[wid];
            Data->WordsMInfo.WordAcc[wid + 1] = Data->WordsMInfo.WordAcc[wid];

            const TWtringBuf word = SentsInfo.GetWordBuf(wid);
            lowerWord.AssignNoAlias(word.data(), word.size());
            lowerWord.to_lower();
            wordVal.WordLangs = NLemmer::ClassifyLanguageAlpha(word.data(), word.size(), false);
            wordVal.IsStopword = word.size() <= 1 || Cfg.GetStopWordsFilter().IsStopWord(lowerWord);
            wordVal.LowerWord = lowerWord;
            wordVal.ExactMatchedPositions = nop;
            wordVal.NotExactMatchedLemmaIds = nop;
            wordVal.SynMatchedLemmaIds = nop;
            wordVal.AlmostUserWordsMatchedLemmaIds = nop;

            const auto* wordData = Query.LowerWords.FindPtr(lowerWord);
            if (wordData) {
                wordVal.ExactMatchedPositions = &wordData->WordPositions;
            }

            wordNormalizer.GetNormalizedWords(word, wordVal.WordLangs, normalizedWords);

            bool nex = false;
            bool syn = false;
            bool almostUserWords = false;
            for (const TUtf16String& normalizedWord : normalizedWords) {
                const auto* form = Query.LowerForms.FindPtr(normalizedWord);
                if (form && !nex) {
                    wordVal.NotExactMatchedLemmaIds = &form->LemmaIds;
                    nex = true;
                }
                if (form && form->SynIds && !syn) {
                    wordVal.SynMatchedLemmaIds = &form->SynIds;
                    syn = true;
                }
                if (form && form->AlmostUserWordsIds && !almostUserWords) {
                    wordVal.AlmostUserWordsMatchedLemmaIds = &form->AlmostUserWordsIds;
                    almostUserWords = true;
                }
                if (nex && syn && almostUserWords) {
                    break;
                }
            }

            if (wid > 0) {
                TWtringBuf blanks = SentsInfo.GetBlanksBefore(wid);
                wordVal.WordAfterTerminal = HasTerminal(blanks);
                if (SentsInfo.WordVal[wid].IsSuffix) {
                    Data->WordsMInfo.WordVal[wid - 1].WordAfterTerminal = wordVal.WordAfterTerminal;
                }
            }

            FillPornWeight(wid, normalizedWords);
            FillSegments(wid);
            FillRegionMatch(wid, normalizedWords); // must be called with filled segments
            FillReadability(wid);
            FillAnswerWeight(wid, normalizedWords);
        }

        if (Query.PosCount() > 1) {
            KillLonelyStopWords();
        }

        FillTelephones();
        FillDates();
        FillShortWords();
        FillMatchSumms(); // must be called after KillLonelyStopWords
    }

    void TSentsMatchInfo::FillPornWeight(int wid, const TVector<TUtf16String>& normalizedWords) {
        Data->WordsMInfo.WordAcc[wid + 1].SumUnmatchedByLinkPornCount = Data->WordsMInfo.WordAcc[wid].SumUnmatchedByLinkPornCount;
        if (IsMatch(wid)) {
            return;
        }
        const int sid = SentsInfo.WordId2SentId(wid);
        if (SentsInfo.GetArchiveSent(sid).SourceArc != ARC_LINK) {
            return;
        }
        for (const TUtf16String& normalizedWord : normalizedWords) {
            if (TPornodict::GetDefault().GetWeight(normalizedWord) > 1e-6) {
                ++Data->WordsMInfo.WordAcc[wid + 1].SumUnmatchedByLinkPornCount;
                break;
            }
        }
    }

    void TSentsMatchInfo::FillAnswerWeight(int wid, const TVector<TUtf16String>& normalizedWords) {
        if (normalizedWords.empty()) {
            return;
        }
        auto& wordAcc = Data->WordsMInfo.WordAcc[wid + 1];
        const TUtf16String& token = TryReplaceNumeric(normalizedWords.front(), SentsInfo.GetWordType(wid));

        if (const TAnswerModels* models = Cfg.GetAnswerModels()) {
            wordAcc.SumIsAnswerWeight += models->ApplyWord(token, DocLangId);
        }
    }

    void TSentsMatchInfo::FillSegments(int wid) {
        const NSegments::TSegmentsInfo* segmInfo = SentsInfo.GetSegments();
        if (!segmInfo || !segmInfo->HasData()) {
            return;
        }

        auto& wordAcc = Data->WordsMInfo.WordAcc[wid + 1];

        const int sid = SentsInfo.WordId2SentId(wid);
        const NSegments::TSegmentCIt segm = segmInfo->GetArchiveSegment(SentsInfo.GetArchiveSent(sid));
        if (segmInfo->IsValid(segm)) {
            const bool isMain = segmInfo->IsMain(segm);
            wordAcc.SumSegmentWeight += segm->Weight;

            using namespace NSegm;
            switch (segm->Type) {
            case STP_FOOTER:
                ++wordAcc.SumFooterWords;
                break;
            case STP_CONTENT:
                ++wordAcc.SumContentWords;
                wordAcc.SumMainContentWords += isMain ? 1 : 0;
                break;
            case STP_HEADER:
                ++wordAcc.SumHeaderWords;
                wordAcc.SumMainHeaderWords += isMain ? 1 : 0;
                break;
            case STP_MENU:
                ++wordAcc.SumMenuWords;
                break;
            case STP_REFERAT:
                ++wordAcc.SumReferatWords;
                break;
            case STP_AUX:
                ++wordAcc.SumAuxWords;
                break;
            case STP_LINKS:
                ++wordAcc.SumLinksWords;
                break;
            }
        }
    }

    void TSentsMatchInfo::FillRegionMatch(int wid, const TVector<TUtf16String>& normalizedWords) {
        auto& wordVal = Data->WordsMInfo.WordVal[wid];
        auto& wordAcc = Data->WordsMInfo.WordAcc[wid + 1];
        const auto& wordAccPrev = Data->WordsMInfo.WordAcc[wid];

        wordVal.MatchedPositionsInRegionPhrase = &Data->WordsMInfo.Nop;

        if (!Query.RegionQuery) {
            return;
        }
        const bool isBadSegmentForRegionMatch =
            wordAcc.SumFooterWords > wordAccPrev.SumFooterWords ||
            wordAcc.SumMenuWords > wordAccPrev.SumMenuWords ||
            wordAcc.SumAuxWords > wordAccPrev.SumAuxWords ||
            wordAcc.SumLinksWords > wordAccPrev.SumLinksWords;
        if (isBadSegmentForRegionMatch) {
            return;
        }

        bool matched = false;
        for (const TUtf16String& normalizedWord : normalizedWords) {
            const TVector<int>* poss =
                Query.RegionQuery->Form2Positions.FindPtr(normalizedWord);
            if (poss) {
                wordVal.MatchedPositionsInRegionPhrase = poss;
                matched = true;
                break;
            }
        }
        if (matched) {
            const int bid = wid - Query.RegionQuery->PosCount + 1;
            if (bid >= 0) {
                bool allPosMatched = true;
                for (int pos = 0; pos < Query.RegionQuery->PosCount; ++pos) {
                    bool posMatched = IsIn(*Data->WordsMInfo.WordVal[bid + pos].MatchedPositionsInRegionPhrase, pos);
                    if (!posMatched) {
                        allPosMatched = false;
                        break;
                    }
                }
                if (allPosMatched) {
                    ++wordAcc.SumRegionMatch;
                }
            }
        }
    }

    void TSentsMatchInfo::FillReadability(int wid) {
        auto& wordAcc = Data->WordsMInfo.WordAcc[wid + 1];
        TWtringBuf word = SentsInfo.GetWordBuf(wid);
        TWtringBuf blanks = SentsInfo.GetBlanksBefore(wid);

        int sid = SentsInfo.WordId2SentId(wid);
        if (!SentsInfo.WordVal[wid].IsSuffix) {
            TWtringBuf blanksBefore = SentsInfo.FirstWordIdInSent(sid) == wid ? SentsInfo.GetSentBeginBlanks(sid) : blanks;
            ReadabilityTrash(blanksBefore,
                wordAcc.SumSlashesInGap, wordAcc.SumVertsInGap, wordAcc.SumPunctReadInGap,
                wordAcc.SumPunctBalInGap, wordAcc.SumTrashAsciiInGap, wordAcc.SumTrashUnicodeInGap);
        } else {
            ++wordAcc.SumPunctReadInGap;
        }
        wordAcc.SumSlashesInsideGap = wordAcc.SumSlashesInGap;
        wordAcc.SumVertsInsideGap = wordAcc.SumVertsInGap;
        wordAcc.SumPunctReadInsideGap = wordAcc.SumPunctReadInGap;
        wordAcc.SumPunctBalInsideGap = wordAcc.SumPunctBalInGap;
        wordAcc.SumTrashAsciiInsideGap = wordAcc.SumTrashAsciiInGap;
        wordAcc.SumTrashUnicodeInsideGap = wordAcc.SumTrashUnicodeInGap;
        if (SentsInfo.LastWordIdInSent(sid) == wid) {
            ReadabilityTrash(SentsInfo.GetSentEndBlanks(sid),
                wordAcc.SumSlashesInGap, wordAcc.SumVertsInGap, wordAcc.SumPunctReadInGap,
                wordAcc.SumPunctBalInGap, wordAcc.SumTrashAsciiInGap, wordAcc.SumTrashUnicodeInGap);
        }

        if (wid > 0 && !SentsInfo.WordVal[wid].IsSuffix) {
            wordAcc.SumStrangeGaps += IsStrangeGap(blanks);
            const int trashInGap = TrashInGap(blanks);
            wordAcc.SumTrashInGaps += trashInGap;
            wordAcc.SumPunctInGaps += !trashInGap && !IsSpaceGap(blanks);
        }

        wordAcc.SumCaps += IsCapitalized(word);

        CountAlphaAndDig(word, wordAcc.SumAlphas, wordAcc.SumDigits, wordAcc.SumCyrAlphas);
    }

    static void FillAllAnchorSpans(const TSentsInfo& sentsInfo, TVector<TArchiveZoneSpan>& anchorSpans) {
        anchorSpans.clear();
        bool hasText = false;
        for (int i = 0; i < sentsInfo.SentencesCount(); ++i) {
            if (sentsInfo.GetArchiveSent(i).SourceArc == ARC_TEXT) {
                hasText = true;
                break;
            }
        }
        if (!hasText) {
            return;
        }
        const TVector<TArchiveZoneSpan>& anchor = sentsInfo.GetTextArcMarkup().GetZone(AZ_ANCHOR).Spans;
        anchorSpans.insert(anchorSpans.end(), anchor.begin(), anchor.end());
        const TVector<TArchiveZoneSpan>& anchorInt = sentsInfo.GetTextArcMarkup().GetZone(AZ_ANCHORINT).Spans;
        anchorSpans.insert(anchorSpans.end(), anchorInt.begin(), anchorInt.end());
    }

    static double GetSentAnchorPercent(TForwardInZoneChecker& anchorChecker, const TArchiveSent& sent) {
        if (sent.SourceArc != ARC_TEXT || sent.Sent.empty() || anchorChecker.Empty())
            return 0.0;

        size_t anchorLen = 0;

        const TSentParts parts = IntersectZonesAndSent(anchorChecker, sent, true); //TODO: set punctHack=false?
        for (const TSentPart& part : parts) {
            if (part.InZone) {
                anchorLen += part.Part.size();
            }
        }

        double percent = double(anchorLen) / double(sent.Sent.size());
        Y_ASSERT(0.0 <= percent && percent <= 1.0);
        return percent;
    }

    static void GetTitleLowerWords(const TSentsMatchInfo* titleSentsMatchInfo, THashSet<TWtringBuf>& titleLowerWords, TVector<TWtringBuf>& titleLowerWordsOrdered) {
        if (titleSentsMatchInfo) {
            titleLowerWordsOrdered.reserve(titleSentsMatchInfo->WordsCount());
            for (int i = 0; i < titleSentsMatchInfo->WordsCount(); ++i) {
                titleLowerWords.insert(titleSentsMatchInfo->GetLowerWord(i));
                titleLowerWordsOrdered.push_back(titleSentsMatchInfo->GetLowerWord(i));
            }
        }
    }

    bool TSentsMatchInfo::IsSentRepeatsTitle(int firstWordId, int lastWordId, const THashSet<TWtringBuf>& titleLowerWords, const TVector<TWtringBuf>& titleLowerWordsOrdered) const {
        const ui32 checkAlgo = Cfg.CheckPassageRepeatsTitleAlgo();
        if (!checkAlgo || titleLowerWords.empty()) {
            return false;
        }

        auto titleContainsAllWords = [&]() -> bool {
            for (int i = firstWordId; i <= lastWordId; ++i) {
                if (!titleLowerWords.contains(Data->WordsMInfo.WordVal[i].LowerWord)) {
                    return false;
                }
            }
            return true;
        };

        auto firstWordsCoincide = [&](int numWords = 2) -> bool {
            int lastId = Min<int>(lastWordId, firstWordId + numWords - 1);
            for (int i = firstWordId, j = 0; i <= lastId && j < static_cast<int>(titleLowerWordsOrdered.size()); ++i, ++j) {
                if (Data->WordsMInfo.WordVal[i].LowerWord != titleLowerWordsOrdered[j]) {
                    return false;
                }
            }
            return true;
        };

        if (checkAlgo == 1) {
            return titleContainsAllWords();
        } else if (checkAlgo == 2) {
            return titleContainsAllWords() && firstWordsCoincide();
        }

        return false;
    }

    void TSentsMatchInfo::InitSents()
    {
        const int sentsCount = SentsInfo.SentencesCount();

        Data->SentsMInfo.SentAcc.resize(sentsCount + 1);
        Data->SentsMInfo.SentVal.resize(sentsCount);

        Data->SentsMInfo.FirstMatchSentId = -1;
        Data->SentsMInfo.LastMatchSentId = -1;

        TForwardInZoneChecker textAreaArchiveZoneChecker(SentsInfo.GetTextArcMarkup().GetZone(AZ_TEXTAREA).Spans);

        TVector<TArchiveZoneSpan> anchorSpans;
        FillAllAnchorSpans(SentsInfo, anchorSpans);
        TForwardInZoneChecker anchorZoneChecker(anchorSpans, true);
        double anchorPercent = 0.0;

        THashSet<TWtringBuf> titleLowerWords;
        TVector<TWtringBuf> titleLowerWordsOrdered;
        GetTitleLowerWords(TitleSentsMatchInfo, titleLowerWords, titleLowerWordsOrdered);

        for (int i = 0; i < sentsCount; ++i) {
            auto& sentVal = Data->SentsMInfo.SentVal[i];
            auto& sentAcc = Data->SentsMInfo.SentAcc[i + 1];
            sentAcc = Data->SentsMInfo.SentAcc[i];

            const int w0 = SentsInfo.FirstWordIdInSent(i);
            const int w1 = SentsInfo.LastWordIdInSent(i);
            const int sentLen = w1 - w0 + 1;
            const int exacts = ExactMatchesInRange(w0, w1);
            const int notExacts = NotExactMatchesInRange(w0, w1);
            const int goodWords = MatchesInRange(w0, w1);
            const int badWords = sentLen - goodWords;
            const bool match = exacts || notExacts;
            if (SentsInfo.IsSentIdFirstInArchiveSent(i)) {
                anchorPercent = GetSentAnchorPercent(anchorZoneChecker, SentsInfo.GetArchiveSent(i));
            }
            const bool isNavlike = exacts + notExacts == 0 && sentLen < 4 && anchorPercent > 0.5;

            sentVal.LongestChain = GetLongestMatchChainInSpanRange(w0, w1);
            sentVal.LooksLikeDefinition = LooksLikeDefinition(*this, i);
            sentVal.RepeatsTitle = IsSentRepeatsTitle(w0, w1, titleLowerWords, titleLowerWordsOrdered);
            sentAcc.SumHasMatches += (exacts || notExacts);
            sentAcc.SumNavlike += isNavlike;
            sentAcc.SumAnchorPercent += anchorPercent;
            if (Data->SentsMInfo.FirstMatchSentId == -1 && match)
                Data->SentsMInfo.FirstMatchSentId = i;
            if (match)
                Data->SentsMInfo.LastMatchSentId = i;

            const int w = sentLen;
            const int strange = StrangeGapsInRange(w0, w1);
            const int trash = TrashInGapsInRange(w0, w1);
            const int punct = PunctGapsInRange(w0, w1);
            const int shorts = ShortsInRange(w0, w1);
            const int caps = CapsInRange(w0, w1) ? CapsInRange(w0, w1) - 1 : 0;
            const double rStrange = strange * 1.0 / w;
            const double rTrash = trash * 1.0 / w;
            const bool noPunct = (punct + 1) * 10 < w && w > 4;
            const double rShorts = shorts * 1.0 / w;
            const int sCaps = caps > 3 ? caps - 1 : 0;
            const double rCaps = sCaps * 1.0 / w;
            const bool lq = rShorts > 0.6
                || rTrash > 0.37
                || rStrange + rCaps + (noPunct ? 0.6 : 0.0) > 1.2;
            const bool mq = w == 1
                || w <= 3 && rShorts + rStrange + rTrash > 0.9;
            const bool infoBonus = (goodWords <= badWords * 1.7 && goodWords > 0 && sentLen > 4);
            const bool infoBonusMatch = (goodWords > 0 && sentLen > 4);
            sentAcc.SumQuality += (lq ? -1 : mq ? 0 : 1);
            sentAcc.SumInfoBonusSents += infoBonus;
            sentAcc.SumInfoBonusMatchSents += infoBonusMatch;

            const NSegments::TSegmentsInfo* segmInfo = SentsInfo.GetSegments();
            if (segmInfo && segmInfo->HasData()) {
                using namespace NSegments;
                const TSegmentCIt curr = segmInfo->GetArchiveSegment(SentsInfo.GetArchiveSent(i));
                if (segmInfo->IsValid(curr)) {
                    if (curr->AdsCSS || curr->AdsHeader) {
                        ++sentAcc.SumAdsSents;
                    }
                    if (curr->IsHeader || curr->HasHeader) {
                        ++sentAcc.SumHeaderSents;
                    }
                    if (curr->MenuCSS) {
                        ++sentAcc.SumMenuSents;
                    }
                    if (curr->PollCSS) {
                        ++sentAcc.SumPollSents;
                    }
                    if (curr->Type == NSegm::STP_FOOTER) {
                        ++sentAcc.SumFooterSents;
                    }
                }
            }

            const TArchiveSent& arcSent = SentsInfo.GetArchiveSent(i);
            if (arcSent.SourceArc == ARC_TEXT && textAreaArchiveZoneChecker.SeekToSent(arcSent.SentId)) {
                ++sentAcc.SumTextAreaSents;
            }
        }
    }

    bool TSentsMatchInfo::IsPureStopWordMatch(int i) const
    {
        if (IsExactMatch(i)) {
            for (int pos : GetExactMatchedPositions(i)) {
                if (!Query.Positions[pos].IsStopWord) {
                    return false;
                }
            }
            return true;
        }
        if (IsNotExactMatch(i)) {
            for (int lemmaId : GetNotExactMatchedLemmaIds(i)) {
                if (!Query.Lemmas[lemmaId].LemmaIsPureStopWord) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    int TSentsMatchInfo::GetLongestMatchChainInSpanRange(int i, int j) const
    {
        int res = 0;
        int cur = 0;
        int lastMatchPos = -1;
        TVector<bool>& lemm = Data->WordsMInfo.TmpLemm;
        Y_ASSERT(lemm.ysize() == Query.IdsCount());
        TVector<bool>& pos = Data->WordsMInfo.TmpPos;
        Y_ASSERT(pos.ysize() == Query.PosCount());
        for (int k = i; k <= j; ++k) {
            if (MatchesInRange(k)) {
                const bool inLastChain = (cur > 0) && (k < lastMatchPos + 3);
                if (!inLastChain) {
                    cur = 0;
                    Fill(lemm.begin(), lemm.end(), false);
                    Fill(pos.begin(), pos.end(), false);
                }

                bool grow = false;
                for (int mPos : GetExactMatchedPositions(k)) {
                    if (!pos[mPos]) {
                        pos[mPos] = true;
                        grow = true;
                    }
                }

                for (int mLemm : GetNotExactMatchedLemmaIds(k)) {
                    if (!lemm[mLemm]) {
                        lemm[mLemm] = true;
                        grow = true;
                    }
                }

                if (grow)
                    ++cur;
                if (cur > res)
                    res = cur;
                lastMatchPos = k;
            }
        }
        return res;
    }

    ESubTGrammar TSentsMatchInfo::GetSubGrammar(int wordId) const {
        const TWtringBuf word = SentsInfo.GetWordBuf(wordId);

        TLangMask langMask = LI_BASIC_LANGUAGES;
        if (DocLangId != LANG_UNK) {
            langMask = TLangMask(DocLangId, LANG_ENG);
        }
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(word.data(), word.size(), lemmas, langMask);
        TGramBitSet gram = TGramBitSet::FromBytes(lemmas[0].GetStemGram());
        if (gram.Has(gVerb)) {
            return STG_VERB;
        }
        if (gram.Has(gAdjective)) {
            return STG_ADJECTIVE;
        }
        if (gram.Has(gSubstantive)) {
            return STG_NOUN;
        }
        if (gram.Has(gConjunction) || gram.Has(gParticle) || gram.Has(gPreposition) || gram.Has(gInterjunction)) {
            return STG_BAD_POS;
        }
        return STG_NONE;
    }

    TSentsMatchInfo::TSentsMatchInfo(const TSentsInfo& sentsInfo, const TQueryy& query, const TConfig& cfg, ELanguage docLangId, const TSentsMatchInfo* titleSentsMatchInfo)
      : SentsInfo(sentsInfo)
      , Query(query)
      , Cfg(cfg)
      , DocLangId(docLangId)
      , TitleSentsMatchInfo(titleSentsMatchInfo)
      , Data(new TData())
    {
        InitWords();
        InitSents();
    }

    TSentsMatchInfo::~TSentsMatchInfo() {
    }

    const TPixelLengthCalculator& TSentsMatchInfo::GetPixelLengthCalculator() const {
        if (!Data->PixelLengthCalculator) {
            Data->PixelLengthCalculator.Reset(new TPixelLengthCalculator(SentsInfo.Text, GetMatchedWords()));
        }
        return *Data->PixelLengthCalculator;
    }
    bool TSentsMatchInfo::IsWordAfterTerminal(int i) const {
        return Data->WordsMInfo.WordVal[i].WordAfterTerminal;
    }
    ESubTGrammar TSentsMatchInfo::GetWordSubGrammar(int i) const { // It's rather expensive function. Use it wise, say for the final candidate only.
        if (Data->WordsMInfo.WordSubGrammar[i] == STG_UNKNOWN) {
            return Data->WordsMInfo.WordSubGrammar[i] = GetSubGrammar(i);
        }
        return Data->WordsMInfo.WordSubGrammar[i];
    }
    const TSentsMatchInfo::TPositionsInQuery& TSentsMatchInfo::GetExactMatchedPositions(int i) const
    {
        return *Data->WordsMInfo.WordVal[i].ExactMatchedPositions;
    }
    const TSentsMatchInfo::TPositionsInQuery& TSentsMatchInfo::GetNotExactMatchedLemmaIds(int i) const
    {
        return *Data->WordsMInfo.WordVal[i].NotExactMatchedLemmaIds;
    }
    const TSentsMatchInfo::TPositionsInQuery& TSentsMatchInfo::GetSynMatchedLemmaIds(int i) const
    {
        return *Data->WordsMInfo.WordVal[i].SynMatchedLemmaIds;
    }
    const TSentsMatchInfo::TPositionsInQuery& TSentsMatchInfo::GetAlmostUserWordsMatchedLemmaIds(int i) const
    {
        return *Data->WordsMInfo.WordVal[i].AlmostUserWordsMatchedLemmaIds;
    }
    int TSentsMatchInfo::WordsCount() const
    {
        return SentsInfo.WordVal.ysize();
    }
    TLangMask TSentsMatchInfo::GetWordLangs(int i) const
    {
        return Data->WordsMInfo.WordVal[i].WordLangs;
    }
    bool TSentsMatchInfo::IsStopword(int i) const
    {
        return Data->WordsMInfo.WordVal[i].IsStopword;
    }
    const TUtf16String& TSentsMatchInfo::GetLowerWord(int i) const
    {
        return Data->WordsMInfo.WordVal[i].LowerWord;
    }
    bool TSentsMatchInfo::IsExactMatch(int i) const
    {
        return !GetExactMatchedPositions(i).empty();
    }
    bool TSentsMatchInfo::IsNotExactMatch(int i) const
    {
        return !IsExactMatch(i) && !GetNotExactMatchedLemmaIds(i).empty();
    }
    bool TSentsMatchInfo::IsMatch(int i) const
    {
        return !GetExactMatchedPositions(i).empty() || !GetNotExactMatchedLemmaIds(i).empty();
    }
    bool TSentsMatchInfo::IsMatch(const TSentWord& i) const
    {
        return IsMatch(i.ToWordId());
    }
    bool TSentsMatchInfo::IsMatch(const TSentMultiword& i) const
    {
        return MatchesInRange(i.GetFirst(), i.GetLast()) > 0;
    }
    bool TSentsMatchInfo::IsExactUserPhone(int i) const
    {
        return Data->WordsMInfo.WordVal[i].MIsExactUserPhone;
    }
    int TSentsMatchInfo::MatchesInRange(int i) const
    {
        return MatchesInRange(i, i);
    }
    int TSentsMatchInfo::MatchesInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumMatch - Data->WordsMInfo.WordAcc[i].SumMatch);
    }
    int TSentsMatchInfo::MatchesInRange(const TSentWord& i, const TSentWord& j) const
    {
        return MatchesInRange(i.ToWordId(), j.ToWordId());
    }
    int TSentsMatchInfo::MatchesInRange(const TSentMultiword& i, const TSentMultiword& j) const
    {
        return MatchesInRange(i.GetFirst(), j.GetLast());
    }
    int TSentsMatchInfo::ExactMatchesInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumExactMatch - Data->WordsMInfo.WordAcc[i].SumExactMatch);
    }
    int TSentsMatchInfo::NotExactMatchesInRange(int i, int j) const
    {
        return MatchesInRange(i, j) - ExactMatchesInRange(i, j);
    }
    int TSentsMatchInfo::ShortsInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumShorts - Data->WordsMInfo.WordAcc[i].SumShorts);
    }
    int TSentsMatchInfo::StrangeGapsInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumStrangeGaps - Data->WordsMInfo.WordAcc[i + 1].SumStrangeGaps);
    }
    int TSentsMatchInfo::TrashInGapsInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumTrashInGaps - Data->WordsMInfo.WordAcc[i + 1].SumTrashInGaps);
    }
    int TSentsMatchInfo::PunctGapsInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumPunctInGaps - Data->WordsMInfo.WordAcc[i + 1].SumPunctInGaps);
    }
    int TSentsMatchInfo::SlashesInGapsInRange(int i, int j) const
    {
        if (SentsInfo.IsWordIdFirstInSent(i))
            return Data->WordsMInfo.WordAcc[j + 1].SumSlashesInGap - Data->WordsMInfo.WordAcc[i].SumSlashesInGap;
        else
            return Data->WordsMInfo.WordAcc[j + 1].SumSlashesInGap - Data->WordsMInfo.WordAcc[i + 1].SumSlashesInsideGap;
    }
    int TSentsMatchInfo::VertsInGapsInRange(int i, int j) const
    {
        if (SentsInfo.IsWordIdFirstInSent(i))
            return Data->WordsMInfo.WordAcc[j + 1].SumVertsInGap - Data->WordsMInfo.WordAcc[i].SumVertsInGap;
        else
            return Data->WordsMInfo.WordAcc[j + 1].SumVertsInGap - Data->WordsMInfo.WordAcc[i + 1].SumVertsInsideGap;
    }
    int TSentsMatchInfo::PunctReadInGapInRange(int i, int j) const
    {
        if (SentsInfo.IsWordIdFirstInSent(i))
            return Data->WordsMInfo.WordAcc[j + 1].SumPunctReadInGap - Data->WordsMInfo.WordAcc[i].SumPunctReadInGap;
        else
            return Data->WordsMInfo.WordAcc[j + 1].SumPunctReadInGap - Data->WordsMInfo.WordAcc[i + 1].SumPunctReadInsideGap;
    }
    int TSentsMatchInfo::PunctBalInGapInRange(int i, int j) const
    {
        if (SentsInfo.IsWordIdFirstInSent(i))
            return Data->WordsMInfo.WordAcc[j + 1].SumPunctBalInGap - Data->WordsMInfo.WordAcc[i].SumPunctBalInGap;
        else
            return Data->WordsMInfo.WordAcc[j + 1].SumPunctBalInGap - Data->WordsMInfo.WordAcc[i + 1].SumPunctBalInsideGap;
    }
    int TSentsMatchInfo::PunctBalInGapInsideRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumPunctBalInsideGap - Data->WordsMInfo.WordAcc[i + 1].SumPunctBalInsideGap);
    }
    int TSentsMatchInfo::TrashAsciiInGapInRange(int i, int j) const
    {
        if (SentsInfo.IsWordIdFirstInSent(i))
            return Data->WordsMInfo.WordAcc[j + 1].SumTrashAsciiInGap - Data->WordsMInfo.WordAcc[i].SumTrashAsciiInGap;
        else
            return Data->WordsMInfo.WordAcc[j + 1].SumTrashAsciiInGap - Data->WordsMInfo.WordAcc[i + 1].SumTrashAsciiInsideGap;
    }
    int TSentsMatchInfo::TrashUTFInGapInRange(int i, int j) const
    {
        if (SentsInfo.IsWordIdFirstInSent(i))
            return Data->WordsMInfo.WordAcc[j + 1].SumTrashUnicodeInGap - Data->WordsMInfo.WordAcc[i].SumTrashUnicodeInGap;
        else
            return Data->WordsMInfo.WordAcc[j + 1].SumTrashUnicodeInGap - Data->WordsMInfo.WordAcc[i + 1].SumTrashUnicodeInsideGap;
    }
    int TSentsMatchInfo::CapsInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumCaps - Data->WordsMInfo.WordAcc[i].SumCaps);
    }
    int TSentsMatchInfo::LangMatchsInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumLangMatchs - Data->WordsMInfo.WordAcc[i].SumLangMatchs);
    }
    int TSentsMatchInfo::AlphasInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumAlphas - Data->WordsMInfo.WordAcc[i].SumAlphas);
    }
    int TSentsMatchInfo::CyrAlphasInRange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumCyrAlphas - Data->WordsMInfo.WordAcc[i].SumCyrAlphas);
    }
    int TSentsMatchInfo::DigitsInrange(int i, int j) const
    {
        return (Data->WordsMInfo.WordAcc[j + 1].SumDigits - Data->WordsMInfo.WordAcc[i].SumDigits);
    }
    //! Calculates telephone zones count in specified word range
    //! Zone is counted only if both of it bounds belongs to the range
    int TSentsMatchInfo::TelephonesInRange(int i, int j) const
    {
        return Max(Data->WordsMInfo.WordAcc[j + 1].SumTelephoneWordEnd - Data->WordsMInfo.WordAcc[i].SumTelephoneWordBegin, 0);
    }
    //! Calculates date zones count in specified word range
    //! Zone is counted only if both of it bounds belongs to the range
    int TSentsMatchInfo::DatesInRange(int i, int j) const
    {
        return Max(Data->WordsMInfo.WordAcc[j + 1].SumDateWordEnd - Data->WordsMInfo.WordAcc[i].SumDateWordBegin, 0);
    }
    double TSentsMatchInfo::GetFooterWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumFooterWords - Data->WordsMInfo.WordAcc[i].SumFooterWords;
    }
    double TSentsMatchInfo::GetContentWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumContentWords - Data->WordsMInfo.WordAcc[i].SumContentWords;
    }
    double TSentsMatchInfo::GetMainContentWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumMainContentWords - Data->WordsMInfo.WordAcc[i].SumMainContentWords;
    }
    double TSentsMatchInfo::GetSegmentWeightSums(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumSegmentWeight - Data->WordsMInfo.WordAcc[i].SumSegmentWeight;
    }
    double TSentsMatchInfo::GetHeaderWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumHeaderWords - Data->WordsMInfo.WordAcc[i].SumHeaderWords;
    }
    double TSentsMatchInfo::GetMainHeaderWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumMainHeaderWords - Data->WordsMInfo.WordAcc[i].SumMainHeaderWords;
    }
    double TSentsMatchInfo::GetMenuWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumMenuWords - Data->WordsMInfo.WordAcc[i].SumMenuWords;
    }
    double TSentsMatchInfo::GetReferatWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumReferatWords - Data->WordsMInfo.WordAcc[i].SumReferatWords;
    }
    double TSentsMatchInfo::GetAuxWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumAuxWords - Data->WordsMInfo.WordAcc[i].SumAuxWords;
    }
    double TSentsMatchInfo::GetLinksWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumLinksWords - Data->WordsMInfo.WordAcc[i].SumLinksWords;
    }
    double TSentsMatchInfo::GetAnswerWeight(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumIsAnswerWeight - Data->WordsMInfo.WordAcc[i].SumIsAnswerWeight;
    }
    int TSentsMatchInfo::GetKandziWords(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumKandziWords - Data->WordsMInfo.WordAcc[i].SumKandziWords;
    }
    bool TSentsMatchInfo::RegionMatchInRange(int i, int j) const
    {
        if (!Query.RegionQuery.Get() || j - i + 1 < Query.RegionQuery->PosCount || (i + Query.RegionQuery->PosCount) < 1) {
            return false;
        }
        return Data->WordsMInfo.WordAcc[j + 1].SumRegionMatch - Data->WordsMInfo.WordAcc[i + Query.RegionQuery->PosCount - 1].SumRegionMatch > 0;
    }
    int TSentsMatchInfo::GetUnmatchedByLinkPornCountInRange(int i, int j) const
    {
        return Data->WordsMInfo.WordAcc[j + 1].SumUnmatchedByLinkPornCount - Data->WordsMInfo.WordAcc[i].SumUnmatchedByLinkPornCount;
    }

    int TSentsMatchInfo::GetFirstMatchSentId() const {
        return Data->SentsMInfo.FirstMatchSentId;
    }
    int TSentsMatchInfo::GetLastMatchSentId() const {
        return Data->SentsMInfo.LastMatchSentId;
    }
    const TEQInfo& TSentsMatchInfo::GetSentEQInfos(int i) const {
        auto& eqInfo = Data->SentsMInfo.SentVal[i].EQInfo;
        if (!eqInfo) {
            eqInfo.Reset(new TEQInfo(*this, SentsInfo.FirstWordIdInSent(i), SentsInfo.LastWordIdInSent(i)));
        }
        return *eqInfo;
    }
    int TSentsMatchInfo::GetSentQuality(int i) const {
        return Data->SentsMInfo.SentAcc[i + 1].SumQuality - Data->SentsMInfo.SentAcc[i].SumQuality;
    }
    bool TSentsMatchInfo::SentHasMatches(int i) const
    {
        return Data->SentsMInfo.SentAcc[i + 1].SumHasMatches - Data->SentsMInfo.SentAcc[i].SumHasMatches;
    }
    bool TSentsMatchInfo::SentLooksLikeDefinition(int i) const
    {
        return Data->SentsMInfo.SentVal[i].LooksLikeDefinition;
    }
    bool TSentsMatchInfo::SentRepeatsTitle(int i) const
    {
        return Data->SentsMInfo.SentVal[i].RepeatsTitle;
    }

    bool TSentsMatchInfo::SentsHasMatches() const
    {
        return Data->SentsMInfo.SentAcc.back().SumHasMatches;
    }

    int TSentsMatchInfo::SumQualityInSentRange(int si, int sj) const {
        return Data->SentsMInfo.SentAcc[sj + 1].SumQuality - Data->SentsMInfo.SentAcc[si].SumQuality;
    }

    int TSentsMatchInfo::NavlikeInSentRange(int si, int sj) const {
        return Data->SentsMInfo.SentAcc[sj + 1].SumNavlike - Data->SentsMInfo.SentAcc[si].SumNavlike;
    }

    double TSentsMatchInfo::SumAnchorPercentInSentRange(int startSent, int endSent) const
    {
        return Data->SentsMInfo.SentAcc[endSent + 1].SumAnchorPercent - Data->SentsMInfo.SentAcc[startSent].SumAnchorPercent;
    }

    int TSentsMatchInfo::GetInfoBonusSentsInRange(int i, int j) const
    {
        return Data->SentsMInfo.SentAcc[j + 1].SumInfoBonusSents - Data->SentsMInfo.SentAcc[i].SumInfoBonusSents;
    }

    int TSentsMatchInfo::GetInfoBonusMatchSentsInRange(int i, int j) const
    {
        return Data->SentsMInfo.SentAcc[j + 1].SumInfoBonusMatchSents - Data->SentsMInfo.SentAcc[i].SumInfoBonusMatchSents;
    }

    int TSentsMatchInfo::GetLongestMatchChainInRange(int i, int j) const
    {
        int res = 0;
        if (i > j)
            return res;
        int si = SentsInfo.WordId2SentId(i);
        int sj = SentsInfo.WordId2SentId(j);
        for (int s = si; s <= sj; ++s) {
            const int w0 = SentsInfo.FirstWordIdInSent(s);
            const int w1 = SentsInfo.LastWordIdInSent(s);
            const int ii = Max(i, w0);
            const int jj = Min(j, w1);
            if (ii == w0 && jj == w1) {
                res = Max(res, Data->SentsMInfo.SentVal[s].LongestChain);
            } else {
                res = Max(res, GetLongestMatchChainInSpanRange(ii, jj));
            }
        }
        return res;
    }
    int TSentsMatchInfo::AdsSentsInRange(int i, int j) const {
        return Data->SentsMInfo.SentAcc[j + 1].SumAdsSents - Data->SentsMInfo.SentAcc[i].SumAdsSents;
    }
    int TSentsMatchInfo::HeaderSentsInRange(int i, int j) const {
        return Data->SentsMInfo.SentAcc[j + 1].SumHeaderSents - Data->SentsMInfo.SentAcc[i].SumHeaderSents;
    }
    int TSentsMatchInfo::PollSentsInRange(int i, int j) const {
        return Data->SentsMInfo.SentAcc[j + 1].SumPollSents - Data->SentsMInfo.SentAcc[i].SumPollSents;
    }
    int TSentsMatchInfo::MenuSentsInRange(int i, int j) const {
        return Data->SentsMInfo.SentAcc[j + 1].SumMenuSents - Data->SentsMInfo.SentAcc[i].SumMenuSents;
    }
    int TSentsMatchInfo::FooterSentsInRange(int i, int j) const {
        return Data->SentsMInfo.SentAcc[j + 1].SumFooterSents - Data->SentsMInfo.SentAcc[i].SumFooterSents;
    }
    int TSentsMatchInfo::TextAreaSentsInRange(int i, int j) const {
        return Data->SentsMInfo.SentAcc[j + 1].SumTextAreaSents - Data->SentsMInfo.SentAcc[i].SumTextAreaSents;
    }
    TVector<TBoldSpan> TSentsMatchInfo::GetMatchedWords() const {
        TVector<TBoldSpan> matchedWords;
        for (int i = 0; i < SentsInfo.WordVal.ysize(); ++i) {
            if (IsMatch(i) || !Data->WordsMInfo.WordVal[i].MatchedPositionsInRegionPhrase->empty()) {
                matchedWords.push_back(TBoldSpan(SentsInfo.WordVal[i].Word.Ofs, SentsInfo.WordVal[i].Word.EndOfs()));
            }
        }
        return matchedWords;
    }
}
