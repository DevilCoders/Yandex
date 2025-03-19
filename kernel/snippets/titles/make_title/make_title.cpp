#include "make_title.h"

#include "util_title.h"


#include <kernel/snippets/config/config.h>
#include <kernel/snippets/idl/enums.h>
#include <kernel/snippets/plm/plm.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_info/sentword.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/smartcut/pixel_length.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/smartcut/consts.h>
#include <kernel/snippets/wordstat/wordstat.h>
#include <kernel/snippets/wordstat/wordstat_data.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/untranslit/untranslit.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/string/strip.h>
#include <library/cpp/string_utils/url/url.h>
#include <utility>
#include <util/string/split.h>
#include <util/string/cast.h>

namespace NSnippets
{
namespace
{

class TWordIter
{
private:
    TSentMultiword Multi;
    TSentWord Single;
    const bool IsMulti;
public:
    TWordIter(const TSentWord& w, bool multi)
        : Multi(w)
        , Single(w)
        , IsMulti(multi)
    {
    }
    bool operator!=(const TWordIter& b) const {
        return IsMulti ? (Multi != b.Multi) : (Single != b.Single);
    }
    int LastWordId() const {
        return IsMulti ? Multi.LastWordId() : Single.LastWordId();
    }
    int FirstWordId() const {
        return IsMulti ? Multi.FirstWordId() : Single.FirstWordId();
    }
    TWordIter& operator++() {
        if (IsMulti) {
            ++Multi;
        } else {
            ++Single;
        }
        return *this;
    }
    TWordIter& operator--() {
        if (IsMulti) {
            --Multi;
        } else {
            --Single;
        }
        return *this;
    }
    static TWordIter Begin(const TSentsInfo& si, bool multi) {
        return TWordIter(si.Begin<TSentWord>(), multi);
    }
    static TWordIter End(const TSentsInfo& si, bool multi) {
        return TWordIter(si.End<TSentWord>(), multi);
    }
    static TWordIter FromWord(const TSentsInfo& si, int wordId, bool multi) {
        return TWordIter(si.WordId2SentWord(wordId), multi);
    }
};

struct TDefinitionCand
{
public:
    int DefFirstWord = 0;
    int DefLastWord = 0;
    size_t DefBeginOffset = 0;
    size_t DefEndOffset = 0;
    EDefSearchResultType Type = NOT_FOUND;

public:
    TWtringBuf GetBuf(const TUtf16String& text) const {
        return TWtringBuf(text.data() + DefBeginOffset, text.data() + DefEndOffset);
    }
};

const wchar16 EM_DASH = wchar16(0x2014);
const TUtf16String DEF_DELIMITERS = u"-:=/*|" + TUtf16String(EM_DASH);

bool IsDefDelimiter(wchar16 c)
{
    return DEF_DELIMITERS.Contains(c);
}

const TUtf16String INTERNAL_PUNCT = u",:;-";

bool IsSpaceOrPunct(wchar16 c)
{
    return IsWhitespace(c) || INTERNAL_PUNCT.Contains(c);
}

size_t GetLengthWithoutTrailingWhitespace(const TWtringBuf& str)
{
    return StripStringRight(str).length();
}

bool ContainsUnpairedSymbols(const TWtringBuf& str)
{
    int roundBracketsCount = 0; //()
    int squareBracketsCount = 0; //[]
    int curlyBracketsCount = 0; //{}
    int quotes = 0; //""
    for (wchar16 c : str) {
        switch (c) {
        case '(':
            ++roundBracketsCount;
            break;
        case ')':
            --roundBracketsCount;
            break;
        case '[':
            ++squareBracketsCount;
            break;
        case ']':
            --squareBracketsCount;
            break;
        case '{':
            ++curlyBracketsCount;
            break;
        case '}':
            --curlyBracketsCount;
            break;
        case '"':
            ++quotes;
            break;
        }
    }
    return roundBracketsCount || squareBracketsCount || curlyBracketsCount || (quotes % 2);
}

constexpr TStringBuf COM_TR_TOKEN = ".com.tr";

static const size_t MAX_TRANSLITER_INPUT_LENGTH = 15;
static const size_t MAX_TRANSLITERATIONS_NUMBER = 30;
static const size_t MIN_HOST_TOKEN_LENGTH = 4;

bool ContainsToken(const TUtf16String& titleToken, const TVector<TUtf16String>& hostTokens) {
    for (size_t i = 0; i < hostTokens.size(); ++i) {
        if (hostTokens[i].size() < MIN_HOST_TOKEN_LENGTH) {
            continue;
        }
        if (titleToken.Contains(hostTokens[i])) {
            return true;
        }
    }
    return false;
}

bool ContainsHostname(const TWtringBuf& defPart, const TString& hostname, bool russianDefinitions)
{
    size_t hostnameSize = hostname.size();
    if (hostname.EndsWith(COM_TR_TOKEN)) {
        hostnameSize -= COM_TR_TOKEN.size();
    } else {
        size_t periodPosition = hostname.rfind('.');
        if (periodPosition != TString::npos) {
            hostnameSize = periodPosition;
        }
    }
    TString defPartGoodSymbols = GetLatinLettersAndDigits(defPart);
    TStringBuf sHost(hostname.data(), hostnameSize);
    if (defPartGoodSymbols.Contains(sHost)) {
        return true;
    }
    if (russianDefinitions) {
        TUtf16String defPartFiltered;
        for (wchar16 c : defPart) {
            wchar16 symbol = ToLower(c);
            if (IsAsciiAlpha(symbol) || (symbol >= 0x400 && symbol <= 0x4FF)) {
                defPartFiltered.push_back(symbol);
            }
        }
        if (defPartFiltered.size() > MAX_TRANSLITER_INPUT_LENGTH) {
            return false;
        }
        TVector<TUtf16String> hostTokens;
        StringSplitter(UTF8ToWide(sHost)).Split(u'.').SkipEmpty().Collect(&hostTokens);
        if (ContainsToken(defPartFiltered, hostTokens)) {
            return true;
        }
        TAutoPtr<TUntransliter> transliter(NLemmer::GetLanguageById(LANG_RUS)->GetTransliter(defPartFiltered));
        TUntransliter::WordPart transliteration = transliter->GetNextAnswer();
        for (size_t i = 0; i < MAX_TRANSLITERATIONS_NUMBER && !transliteration.Empty(); ++i) {
            const TUtf16String& currentTransliteration = transliteration.GetWord();
            if (ContainsToken(currentTransliteration, hostTokens)) {
                return true;
            }
            transliteration = transliter->GetNextAnswer();
        }
    }
    return false;
}

size_t FindForwardDefinitionOffset(const TSentsInfo& si, int lastWord, size_t maxDefLength)
{
    TWtringBuf blanks = si.GetBlanksAfter(lastWord);
    while (blanks.size() >= 2) {
        if (IsWhitespace(blanks[blanks.size() - 1]) && IsDefDelimiter(blanks[blanks.size() - 2])) {
            size_t ofs = blanks.data() + blanks.size() - si.Text.data();
            if (ofs >= maxDefLength + 2) {
                return 0;
            }
            return ofs;
        }
        blanks.Chop(1);
    }
    return 0;
}

TDefinitionCand FindForwardDefinition(const TSentsInfo& si, size_t maxDefLength,
    bool multiToken, const TString& hostname, bool russianDefinitions)
{
    TDefinitionCand bestDef;
    TDefinitionCand def;

    def.DefBeginOffset = 0;
    def.DefFirstWord = 0;

    const TWordIter beginIt = TWordIter::Begin(si, multiToken);
    const TWordIter endIt = TWordIter::End(si, multiToken);
    for (TWordIter it = beginIt; it != endIt; ++it) {
        def.DefLastWord = it.LastWordId();
        if (si.WordVal[def.DefLastWord].Word.EndOfs() >= maxDefLength) {
            break;
        }
        if (!hostname.empty()) {
            def.DefEndOffset = FindForwardDefinitionOffset(si, def.DefLastWord, maxDefLength);
            if (def.DefEndOffset && ContainsHostname(def.GetBuf(si.Text), hostname, russianDefinitions)) {
                bestDef = def;
                break;
            }
        } else {
            def.DefEndOffset = FindForwardDefinitionOffset(si, def.DefLastWord, maxDefLength);
            if (def.DefEndOffset) {
                bestDef = def;
            }
        }
    }

    if (!bestDef.DefEndOffset || ContainsUnpairedSymbols(bestDef.GetBuf(si.Text))) {
        return TDefinitionCand();
    }
    bestDef.Type = FORWARD;
    return bestDef;
}

size_t FindBackwardDefinitionOffset(const TSentsInfo& si, int firstWord)
{
    if (firstWord == 0) {
        return 0;
    }
    TWtringBuf blanks = si.GetBlanksBefore(firstWord);
    while (blanks.size() >= 2) {
        if (IsWhitespace(blanks[blanks.size() - 2]) && IsDefDelimiter(blanks[blanks.size() - 1])) {
            return blanks.data() + blanks.size() - 2 - si.Text.data();
        }
        blanks.Chop(1);
    }
    return 0;
}

bool IsBadBackwardDefinition(const TSentsMatchInfo& smi, const TDefinitionCand& def)
{
    const int matchesCount = smi.MatchesInRange(def.DefFirstWord, def.DefLastWord);
    if (matchesCount == 0 || matchesCount <= def.DefLastWord - def.DefFirstWord) {
        return true;
    }
    TWordStat stat(smi.Query, smi);
    stat.SetSpan(0, def.DefFirstWord - 1);
    double partIdf = stat.Data().SumPosIdfNorm;
    stat.AddSpan(def.DefFirstWord, def.DefLastWord);
    double fullIdf = stat.Data().SumPosIdfNorm;
    const double IDF_EPS = 1e-10;
    if (Abs(partIdf - fullIdf) < IDF_EPS) {
        return true;
    }
    return false;
}

TDefinitionCand FindBackwardDefinition(const TSentsMatchInfo& smi, size_t maxDefLength,
    bool multiToken, const TString& hostname, bool russianDefinitions)
{
    const TSentsInfo& si = smi.SentsInfo;
    TDefinitionCand bestDef;
    TDefinitionCand def;

    Y_ASSERT(si.WordCount() > 0);
    def.DefEndOffset = GetLengthWithoutTrailingWhitespace(si.Text);
    def.DefLastWord = si.WordCount() - 1;

    size_t wordCount = 0;
    const TWordIter beginIt = TWordIter::Begin(si, multiToken);
    const TWordIter endIt = TWordIter::End(si, multiToken);
    for (TWordIter it = endIt; it != beginIt;) {
        --it;
        def.DefFirstWord = it.FirstWordId();
        if (!hostname.empty()) {
            if (si.WordVal[def.DefFirstWord].Word.Ofs + maxDefLength < def.DefEndOffset) {
                break;
            }
            def.DefBeginOffset = FindBackwardDefinitionOffset(si, def.DefFirstWord);
            if (def.DefBeginOffset && ContainsHostname(def.GetBuf(si.Text), hostname, russianDefinitions)) {
                bestDef = def;
                break;
            }
        } else {
            if (!smi.IsStopword(def.DefFirstWord)) {
                ++wordCount;
                if (wordCount > 2) {
                    break;
                }
            }
            def.DefBeginOffset = FindBackwardDefinitionOffset(si, def.DefFirstWord);
            if (def.DefBeginOffset) {
                bestDef = def;
            }
        }
    }

    if (!bestDef.DefBeginOffset || ContainsUnpairedSymbols(bestDef.GetBuf(si.Text))) {
        return TDefinitionCand();
    }
    if (hostname.empty() && IsBadBackwardDefinition(smi, bestDef)) {
        return TDefinitionCand();
    }
    bestDef.Type = BACKWARD;
    return bestDef;
}

TDefinitionCand FindDefinition(const TSentsMatchInfo& smi, const TConfig& cfg, const TMakeTitleOptions& options)
{
    const TSentsInfo& si = smi.SentsInfo;
    size_t maxDefLength = options.MaxDefinitionLen;
    bool multiToken = !options.AllowBreakMultiToken;
    bool russianDefinitions = !cfg.UseTurkey();

    if (options.DefinitionMode == TDM_IGNORE || maxDefLength == 0 || smi.WordsCount() == 0) {
        return TDefinitionCand();
    }

    TString hostname;
    if (options.HostnamesForDefinitions && options.Url) {
        hostname = ToString(CutWWWPrefix(GetOnlyHost(options.Url)));
    }

    TDefinitionCand def = FindForwardDefinition(si, maxDefLength, multiToken, hostname, russianDefinitions);
    if (def.Type == NOT_FOUND) {
        def = FindBackwardDefinition(smi, maxDefLength, multiToken, hostname, russianDefinitions);
    }
    if (def.Type == NOT_FOUND) {
        return TDefinitionCand();
    }
    return def;
}

TWordIter GetBeginWordIter(const TSentsInfo& si, const TDefinitionCand& definition, bool multiToken)
{
    if (definition.Type == FORWARD) {
        return ++TWordIter::FromWord(si, definition.DefLastWord, multiToken);
    }
    return TWordIter::Begin(si, multiToken);
}

TWordIter GetEndWordIter(const TSentsInfo& si, const TDefinitionCand& definition, bool multiToken)
{
    if (definition.Type == BACKWARD) {
        return TWordIter::FromWord(si, definition.DefFirstWord, multiToken);
    }
    return TWordIter::End(si, multiToken);
}

size_t GetWordBeginOffset(const TSentsInfo& si, const TDefinitionCand& definition, int wordId)
{
    const size_t forwardDefLen = definition.Type == FORWARD ? definition.DefEndOffset : 0;
    if (wordId == 0 || (definition.Type == FORWARD && definition.DefLastWord + 1 == wordId)) {
        return forwardDefLen;
    }
    size_t beginOfs = si.WordVal[wordId].Word.Ofs;
    // TODO: correct cutting
    while (beginOfs > forwardDefLen && !IsWhitespace(si.Text[beginOfs - 1])) {
        --beginOfs;
    }
    return beginOfs;
}

const TUtf16String GOOD_FIRST_SYMBOLS = u"([{\"«“";

bool IsGoodFirstSymbol(wchar16 c)
{
    return GOOD_FIRST_SYMBOLS.Contains(c);
}

const TUtf16String GOOD_LAST_SYMBOLS = u".!?)]}»”";

bool IsGoodLastSymbol(wchar16 c)
{
    return GOOD_LAST_SYMBOLS.Contains(c);
}

size_t GetWordEndOffset(const TSentsInfo& si, const TDefinitionCand& definition, int wordId, bool readabilityExp)
{
    if (wordId + 1 == si.WordCount()) {
        return GetLengthWithoutTrailingWhitespace(si.Text);
    }
    if (definition.Type == BACKWARD && wordId + 1 == definition.DefFirstWord) {
        return definition.DefBeginOffset;
    }
    const size_t wordEndOfs = si.WordVal[wordId].Word.EndOfs();
    const size_t nextWordOfs = si.WordVal[wordId + 1].Word.Ofs;
    Y_ASSERT(nextWordOfs <= si.Text.size());
    size_t endOfs = wordEndOfs;
    if (readabilityExp) {
        if (endOfs < nextWordOfs && (si.Text[endOfs] == '\'' || si.Text[endOfs] == '"') ) {
            ++endOfs;
        }
        while (endOfs < nextWordOfs && IsSpace(si.Text[endOfs])) {
            ++endOfs;
        }
        if (endOfs >= nextWordOfs || !IsGoodLastSymbol(si.Text[endOfs])) {
            return wordEndOfs;
        }
        while (endOfs < nextWordOfs && IsGoodLastSymbol(si.Text[endOfs])) {
            ++endOfs;
        }
    } else {
        while (endOfs < nextWordOfs && !IsSpaceOrPunct(si.Text[endOfs])) {
            ++endOfs;
        }
    }
    return endOfs;
}

struct TTitlePart
{
public:
    bool IsValid = false;
    double Weight = 0.0;
    double WizardWeight = 0.0;
    int FirstWord = 0;
    int LastWord = 0;
    size_t BeginOffset = 0;
    size_t EndOffset = 0;
    bool BeginDots = false;
    bool EndDots = false;
    bool ReadableBreak = false;
    double LogMatchIdfSum = 0.0;
    double MatchUserIdfSum = 0.0;
    int SynonymsCount = 0;
    double QueryWordsRatio = 0.0;
    bool HasRegionMatch = false;
    wchar16 ClosingSymbol = wchar16(0);
};

int GetTitleSymbolLength(const TTitlePart& part, const TDefinitionCand& definition)
{
    int len = 0;
    if (definition.Type != NOT_FOUND) {
        len += definition.DefEndOffset - definition.DefBeginOffset;
    }
    if (part.BeginDots) {
        len += BOUNDARY_ELLIPSIS.length();
    }
    len += part.EndOffset - part.BeginOffset;
    if (part.ClosingSymbol) {
        ++len;
    }
    if (part.EndDots) {
        len += BOUNDARY_ELLIPSIS.length();
    }
    return len;
}

class TTitlePixelLength
{
private:
    float FontSize = 0.0f;
    int PixelsInLine = 0;
    TPixelLengthCalculator Calculator;

public:
    TTitlePixelLength(const TSentsMatchInfo& smi, float fontSize, int pixelsInLine)
        : FontSize(fontSize)
        , PixelsInLine(pixelsInLine)
        , Calculator(smi.SentsInfo.Text, smi.GetMatchedWords())
    {
    }

    float CalcTitleLengthInRows(const TTitlePart& part, const TDefinitionCand& definition) const {
        float lengthInRows = 0.0f;

        if (definition.Type == FORWARD) {
            lengthInRows = Calculator.CalcLengthInRows(definition.DefBeginOffset,
                definition.DefEndOffset, FontSize, PixelsInLine, lengthInRows);
        }

        float prefixLengthInPixels = 0.0f;
        if (part.BeginDots) {
            prefixLengthInPixels += Calculator.GetBoundaryEllipsisLength(FontSize);
        }
        float suffixLengthInPixels = 0.0f;
        if (part.EndDots) {
            suffixLengthInPixels += Calculator.GetBoundaryEllipsisLength(FontSize);
        }
        if (part.ClosingSymbol) {
            suffixLengthInPixels += GetSymbolPixelLength(part.ClosingSymbol, FontSize);
        }
        lengthInRows = Calculator.CalcLengthInRows(part.BeginOffset, part.EndOffset,
            FontSize, PixelsInLine, lengthInRows, prefixLengthInPixels, suffixLengthInPixels);

        if (definition.Type == BACKWARD) {
            lengthInRows = Calculator.CalcLengthInRows(definition.DefBeginOffset,
                definition.DefEndOffset, FontSize, PixelsInLine, lengthInRows);
        }

        return lengthInRows;
    }
};

bool TitleFitsInLength(const TTitlePart& part, const TDefinitionCand& definition,
    const TTitlePixelLength& tpl, const TMakeTitleOptions& options)
{
    if (options.TitleCutMethod == TCM_SYMBOL) {
        return GetTitleSymbolLength(part, definition) <= options.MaxTitleLen;
    } else {
        return tpl.CalcTitleLengthInRows(part, definition) <= options.MaxTitleLengthInRows;
    }
}

TUtf16String GetTitleString(const TTitlePart& part, const TDefinitionCand& definition,
    const TSentsInfo& si)
{
    TUtf16String titleString;
    if (definition.Type == FORWARD) {
        titleString.append(definition.GetBuf(si.Text));
    }
    if (part.BeginDots) {
        titleString.append(BOUNDARY_ELLIPSIS);
    }
    titleString.append(si.Text.data() + part.BeginOffset, si.Text.data() + part.EndOffset);
    if (part.EndDots) {
        titleString.append(BOUNDARY_ELLIPSIS);
    }
    if (part.ClosingSymbol) {
        titleString.push_back(part.ClosingSymbol);
    }
    if (definition.Type == BACKWARD) {
        titleString.append(definition.GetBuf(si.Text));
    }
    return titleString;
}

bool CurrentIsBetter(const TTitlePart& curPart, const TTitlePart& bestPart, const TConfig& cfg)
{
    if (!bestPart.IsValid) {
        return true;
    }
    Y_ASSERT(curPart.FirstWord > bestPart.FirstWord ||
        curPart.FirstWord == bestPart.FirstWord &&
        curPart.LastWord > bestPart.LastWord);
    if (curPart.Weight > bestPart.Weight + 1E-7) {
        return true;
    }
    if (curPart.Weight <= bestPart.Weight - 1E-7) {
        return false;
    }
    if (curPart.FirstWord == bestPart.FirstWord && !cfg.TitlesReadabilityExp()) {
        return true;
    }
    const bool synCountBoost = cfg.SynCount() &&
        curPart.SynonymsCount > bestPart.SynonymsCount &&
        curPart.WizardWeight > 1.2 * bestPart.WizardWeight;
    if (synCountBoost) {
        return true;
    }
    const bool regionBoost = curPart.HasRegionMatch &&
        !bestPart.HasRegionMatch &&
        curPart.WizardWeight > bestPart.WizardWeight - 1E-7;
    if (regionBoost) {
        return true;
    }
    if (curPart.FirstWord == bestPart.FirstWord && cfg.TitlesReadabilityExp()) {
        if (!bestPart.ReadableBreak) {
            return true;
        }
        if (curPart.ReadableBreak) {
            return true;
        }
        Y_ASSERT(curPart.BeginOffset <= curPart.EndOffset);
        size_t curPartLen = curPart.EndOffset - curPart.BeginOffset;
        Y_ASSERT(bestPart.BeginOffset <= bestPart.EndOffset);
        size_t bestPartLen = bestPart.EndOffset - bestPart.BeginOffset;
        if (curPartLen > 1.2 * bestPartLen) {
            return true;
        }
    }
    return false;
}

const TUtf16String SENTENCE_DELIMITERS = u"!?|";

bool IsSentenceDelimiter(wchar16 c, size_t lastWordLen)
{
    return c == '.' ? lastWordLen > 3 : SENTENCE_DELIMITERS.Contains(c);
}

void AnalyzeTitleBeginning(const TSentsInfo& si, TTitlePart& titlePart)
{
    if (!titlePart.BeginDots) {
        return;
    }
    int firstWord = titlePart.FirstWord;
    if (firstWord == 0) {
        return;
    }
    TWtringBuf blanks = si.GetBlanksAfter(firstWord - 1);
    size_t lastWordLen = si.WordVal[firstWord - 1].Word.Len;
    while (blanks.size() >= 2) {
        if (IsWhitespace(blanks[1]) && IsSentenceDelimiter(blanks[0], lastWordLen)) {
            titlePart.BeginDots = false;
            return;
        }
        blanks.Skip(1);
    }
}

void AnalyzeTitleBreak(const TSentsInfo& si, TTitlePart& titlePart)
{
    if (!titlePart.EndDots) {
        return;
    }
    int lastWord = titlePart.LastWord;
    TWtringBuf blanks = si.GetBlanksAfter(lastWord);
    size_t lastWordLen = si.WordVal[lastWord].Word.Len;
    while (blanks.size() >= 2) {
        if (IsWhitespace(blanks[0]) && IsGoodFirstSymbol(blanks[1])) {
            titlePart.ReadableBreak = true;
            return;
        }
        if (IsWhitespace(blanks[1]) && IsSentenceDelimiter(blanks[0], lastWordLen)) {
            titlePart.EndDots = false;
            titlePart.ReadableBreak = true;
            return;
        }
        blanks.Skip(1);
    }
}

struct TPairedSymbols {
    wchar16 OpeningSymbol;
    wchar16 ClosingSymbol;
};

static const TPairedSymbols PAIRED_SYMBOLS[] = {
    {'(', ')'},
    {'[', ']'},
    {'{', '}'},
    {'"', '"'},
    {wchar16(0x00AB), wchar16(0x00BB)}, // « »
    {wchar16(0x201C), wchar16(0x201D)}, // “ ”
};

bool NeedClosingSymbol(const TWtringBuf& titleSuffix, size_t breakPosition, TPairedSymbols pairedSymbols) {
    size_t openingSymbolPosition = titleSuffix.Head(breakPosition).find(pairedSymbols.OpeningSymbol);
    if (openingSymbolPosition == TString::npos) {
        return false;
    }
    size_t closingSymbolPosition = titleSuffix.Tail(openingSymbolPosition + 1).find(pairedSymbols.ClosingSymbol);
    if (closingSymbolPosition == TString::npos || closingSymbolPosition + openingSymbolPosition + 1 < breakPosition) {
        return false;
    }
    if (titleSuffix.Tail(openingSymbolPosition + 1).Head(closingSymbolPosition).Contains(pairedSymbols.OpeningSymbol)) {
        return false;
    }
    return true;
}

TTitlePart FindBestPart(const TSentsMatchInfo& smi, const TConfig& cfg, const TDefinitionCand& definition,
    const TMakeTitleOptions& options, const TTitlePixelLength& tpl, TWordStat& stat)
{
    const TSentsInfo& si = smi.SentsInfo;
    const TQueryy& query = smi.Query;
    bool multiToken = !options.AllowBreakMultiToken;

    bool definitionHasRegionMatch = false;
    bool definitionHasUserLemmas = false;
    if (query.RegionQuery.Get() && definition.Type != NOT_FOUND) {
        if (smi.RegionMatchInRange(definition.DefFirstWord, definition.DefLastWord)) {
            definitionHasRegionMatch = true;
        }
        if (!definitionHasRegionMatch) {
            stat.SetSpan(definition.DefFirstWord, definition.DefLastWord);
            definitionHasUserLemmas = stat.Data().LemmaSeenCount.NonstopUser > 0;
        }
    }

    TTitlePart bestPart;
    TTitlePart part;

    const TWordIter beginIt = GetBeginWordIter(si, definition, multiToken);
    const TWordIter endIt = GetEndWordIter(si, definition, multiToken);
    for (TWordIter firstMWord = beginIt; firstMWord != endIt; ++firstMWord) {
        part.FirstWord = firstMWord.FirstWordId();
        part.BeginOffset = GetWordBeginOffset(si, definition, part.FirstWord);
        part.BeginDots = (firstMWord != beginIt);
        if (options.TitleGeneratingAlgo == TGA_PREFIX_LIKE) {
            AnalyzeTitleBeginning(si, part);
            if (part.BeginDots) {
                continue;
            }
        }

        for (TWordIter lastMWord = firstMWord; lastMWord != endIt; ++lastMWord) {
            const TWordIter nextMWord = ++TWordIter(lastMWord);
            if (nextMWord != endIt && options.TitleGeneratingAlgo == TGA_NAIVE) {
                continue;
            }
            if (nextMWord != endIt && smi.IsStopword(lastMWord.LastWordId())) {
                continue;
            }

            part.LastWord = lastMWord.LastWordId();
            part.EndOffset = GetWordEndOffset(si, definition, part.LastWord, cfg.TitlesReadabilityExp());

            part.EndDots = (nextMWord != endIt);
            part.ReadableBreak = !part.EndDots;
            if (cfg.TitlesReadabilityExp()) {
                AnalyzeTitleBreak(si, part);
            }

            if (options.TitleGeneratingAlgo == TGA_READABLE && part.BeginDots && part.EndDots) {
                continue;
            }

            if (!TitleFitsInLength(part, definition, tpl, options)) {
                break;
            }

            stat.SetSpan(part.FirstWord, part.LastWord);
            if (cfg.DefIdf() && definition.Type != NOT_FOUND) {
                stat.AddSpan(definition.DefFirstWord, definition.DefLastWord);
            }
            part.Weight = cfg.AlmostUserWordsInTitles()
                ? stat.Data().SumAlmostUserWordsPosIdfNorm
                : stat.Data().SumUserPosIdfNorm;
            part.HasRegionMatch = false;
            if (query.RegionQuery.Get() && !definitionHasRegionMatch) {
                if (definitionHasUserLemmas || stat.Data().LemmaSeenCount.NonstopUser > 0) {
                    if (smi.RegionMatchInRange(part.FirstWord, part.LastWord)) {
                        part.HasRegionMatch = true;
                    }
                }
            }
            part.WizardWeight = stat.Data().SumWizardPosIdfNorm;
            part.SynonymsCount = stat.Data().SynWordSeenCount;

            if (CurrentIsBetter(part, bestPart, cfg)) {
                if (!cfg.DefIdf() && definition.Type != NOT_FOUND) {
                    stat.AddSpan(definition.DefFirstWord, definition.DefLastWord);
                }
                part.LogMatchIdfSum = stat.Data().LogMatchWordIdfNormSum();
                part.MatchUserIdfSum = stat.Data().LogUserMatchWordIdfNormSum();
                part.SynonymsCount = stat.Data().SynWordSeenCount; // may change
                part.QueryWordsRatio = (double)stat.Data().LikeWordSeenCount.GetTotal() / stat.Data().Words;
                part.IsValid = true;
                bestPart = part;
            }
        }

        if (options.TitleGeneratingAlgo == TGA_NAIVE || options.TitleGeneratingAlgo == TGA_PREFIX) {
            break;
        }
    }

    if (bestPart.IsValid && cfg.TitlesReadabilityExp()) {
        TWtringBuf titleSuffix(si.Text);
        if (definition.Type == BACKWARD) {
            titleSuffix.Trunc(definition.DefBeginOffset);
        }
        titleSuffix.Skip(bestPart.BeginOffset);
        Y_ASSERT(bestPart.BeginOffset <= bestPart.EndOffset);
        size_t breakPosition = bestPart.EndOffset - bestPart.BeginOffset;
        for (const TPairedSymbols& pairedSymbols : PAIRED_SYMBOLS) {
            if (NeedClosingSymbol(titleSuffix, breakPosition, pairedSymbols)) {
                bestPart.ClosingSymbol = pairedSymbols.ClosingSymbol;
                if (!TitleFitsInLength(bestPart, definition, tpl, options)) {
                    bestPart.ClosingSymbol = wchar16(0);
                    continue;
                }
                break;
            }
        }
    }

    return bestPart;
}

double CalculatePlmScore(const TTitlePart& part, const TDefinitionCand& definition,
    const TSentsMatchInfo& smi)
{
    TPLMStatData plmStat(smi);
    TVector<std::pair<int, int>> fragms;
    if (definition.Type == FORWARD) {
        fragms.push_back(std::make_pair(definition.DefFirstWord, definition.DefLastWord));
    }
    fragms.push_back(std::make_pair(part.FirstWord, part.LastWord));
    if (definition.Type == BACKWARD) {
        fragms.push_back(std::make_pair(definition.DefFirstWord, definition.DefLastWord));
    }
    return plmStat.CalculatePLMScore(fragms);
}

TSnip GetTitleSnip(const TTitlePart& part, const TDefinitionCand& definition,
    const TSentsMatchInfo& smi)
{
    TSnip resultSnip;
    if (definition.Type == FORWARD) {
        resultSnip.Snips.push_back(TSingleSnip(definition.DefFirstWord, definition.DefLastWord, smi));
    }
    resultSnip.Snips.push_back(TSingleSnip(part.FirstWord, part.LastWord, smi));
    if (definition.Type == BACKWARD) {
        resultSnip.Snips.push_back(TSingleSnip(definition.DefFirstWord, definition.DefLastWord, smi));
    }
    if (resultSnip.Snips.size() == 2) {
        const TSingleSnip& snip1 = resultSnip.Snips.front();
        const TSingleSnip& snip2 = resultSnip.Snips.back();
        if (snip1.GetLastWord() + 1 == snip2.GetFirstWord()) {
            TSingleSnip snip(snip1.GetFirstWord(), snip2.GetLastWord(), smi);
            resultSnip.Snips.clear();
            resultSnip.Snips.push_back(snip);
        }
    }
    return resultSnip;
}

TSnipTitle BuildSnipTitle(
    const TDefinitionCand& definition,
    const TTitlePart& bestPart,
    const TMakeTitleOptions& options,
    const TTitlePixelLength& tpl,
    const TRetainedSentsMatchInfo& result)
{
    const TSentsMatchInfo& smi = *result.GetSentsMatchInfo();
    const TSentsInfo& si = *result.GetSentsInfo();

    TTitleFactors titleFactors;
    TSnip resultSnip;
    TUtf16String titleString;

    if (bestPart.IsValid) {
        resultSnip = GetTitleSnip(bestPart, definition, smi);
        titleString = GetTitleString(bestPart, definition, si);
        titleFactors.PixelLength = tpl.CalcTitleLengthInRows(bestPart, definition) * options.PixelsInLine;
        titleFactors.PLMScore = CalculatePlmScore(bestPart, definition, smi);
    }

    titleFactors.LogMatchIdfSum = bestPart.LogMatchIdfSum;
    titleFactors.MatchUserIdfSum = bestPart.MatchUserIdfSum;
    titleFactors.SynonymsCount = bestPart.SynonymsCount;
    titleFactors.QueryWordsRatio = bestPart.QueryWordsRatio;
    titleFactors.HasRegionMatch = bestPart.HasRegionMatch;
    titleFactors.HasBreak = bestPart.BeginDots || bestPart.EndDots;
    TDefinition titleDefinition;
    if (options.HostnamesForDefinitions && definition.Type != NOT_FOUND) {
        titleDefinition = TDefinition(ToWtring(definition.GetBuf(si.Text)), definition.Type);
    }
    return TSnipTitle(titleString, result, resultSnip, titleFactors, titleDefinition);
}

} // anonymous namespace

TMakeTitleOptions::TMakeTitleOptions(const TConfig& cfg)
    : AllowBreakMultiToken(false)
    , TitleGeneratingAlgo(cfg.GetTitleGeneratingAlgo())
    , TitleCutMethod(cfg.GetTitleCutMethod())
    , MaxTitleLengthInRows(cfg.GetMaxTitleLengthInRows())
    , PixelsInLine(cfg.GetTitlePixelsInLine())
    , TitleFontSize(cfg.GetTitleFontSize())
    , MaxTitleLen(cfg.GetMaxTitleLen())
    , MaxDefinitionLen(cfg.GetMaxDefinitionLen())
    , DefinitionMode(cfg.GetDefinitionMode())
    , Url()
    , HostnamesForDefinitions(false)
{
}

TSnipTitle MakeTitle(const TUtf16String& source, const TConfig& cfg, const TQueryy& query, const TMakeTitleOptions& options)
{
    TRetainedSentsMatchInfo result;
    result.SetView(source, TRetainedSentsMatchInfo::TParams(cfg, query));

    TDefinitionCand definition = FindDefinition(*result.GetSentsMatchInfo(), cfg, options);
    if (definition.Type != NOT_FOUND && (options.DefinitionMode == TDM_ELIMINATE)) {
        TUtf16String text(result.GetSentsInfo()->Text);
        text.erase(definition.DefBeginOffset, definition.DefEndOffset - definition.DefBeginOffset);
        // please pay attention that SentsMatchInfo inside the result is changed
        result.SetView(text, TRetainedSentsMatchInfo::TParams(cfg, query));
        definition = TDefinitionCand();
    }

    const TSentsMatchInfo& smi = *result.GetSentsMatchInfo();
    TTitlePixelLength tpl(smi, options.TitleFontSize, options.PixelsInLine);
    TWordStat stat(query, smi);
    TTitlePart bestPart = FindBestPart(smi, cfg, definition, options, tpl, stat);
    if (!bestPart.IsValid && definition.Type != NOT_FOUND) {
        definition = TDefinitionCand();
        bestPart = FindBestPart(smi, cfg, definition, options, tpl, stat);
    }
    return BuildSnipTitle(definition, bestPart, options, tpl, result);
}

TUtf16String MakeTitleString(const TUtf16String& source, const TRichRequestNode* richtreeRoot, int maxPixelLength, float fontSize) {
    TConfig cfg;
    TQueryy query(richtreeRoot, cfg);
    TMakeTitleOptions options(cfg);
    options.TitleCutMethod = TCM_PIXEL;
    options.PixelsInLine = maxPixelLength;
    options.TitleFontSize = fontSize;
    options.DefinitionMode = TDM_IGNORE;
    TUtf16String cleanSource = source;
    ClearChars(cleanSource, /* allowSlashes */ false, cfg.AllowBreveInTitle());
    TSnipTitle title = MakeTitle(cleanSource, cfg, query, options);
    return title.GetTitleString();
}

TUtf16String MakeTitleString(const TUtf16String& source, const TRichRequestNode* richtreeRoot, size_t maxTitleLen) {
    TConfig cfg;
    TQueryy query(richtreeRoot, cfg);
    TMakeTitleOptions options(cfg);
    options.TitleCutMethod = TCM_SYMBOL;
    options.MaxTitleLen = maxTitleLen;
    options.DefinitionMode = TDM_IGNORE;
    TUtf16String cleanSource = source;
    ClearChars(cleanSource, /* allowSlashes */ false, cfg.AllowBreveInTitle());
    TSnipTitle title = MakeTitle(cleanSource, cfg, query, options);
    return title.GetTitleString();
}

} // namespace NSnippets
