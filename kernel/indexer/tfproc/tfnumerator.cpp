#include    <cstdio>
#include    <memory>
#include    <cmath>

#include    <util/generic/string.h>
#include    <library/cpp/deprecated/fgood/fgood.h>
#include    <util/generic/hash.h>
#include    <util/generic/hash_set.h>
#include    <util/memory/tempbuf.h>
#include    <library/cpp/string_utils/url/url.h>
#include    <util/string/printf.h>
#include    <util/generic/singleton.h>

#include    <library/cpp/charset/recyr.hh>
#include    <library/cpp/numerator/numerate.h>
#include    <kernel/remap/remap_table.h>
#include    <kernel/indexer/face/inserter.h>

#include    "tfnumerator.h"
#include    "canonchar.h"
#include    "remap.inc"
#include    "formula.inc"
#include    "factor_remap.inc"
#include    "freqwords.inc"

const float MIN_TR_VAL = -1.095512;
const float MAX_TR_VAL = 0.236264;

const ui32 FNV_PRIME = 0x811C9DC5;

const char* const INTEREST_TAGS[] = {
    "table", "td", "tr", "div", "link", "br", "font", "input", "small",  "strong",
    "li", "a", "img",  "h4", "h5",
};

static_assert(sizeof(INTEREST_TAGS)/sizeof(INTEREST_TAGS[0]) == N_INTEREST_TAGS, "expect sizeof(INTEREST_TAGS)/sizeof(INTEREST_TAGS[0]) == N_INTEREST_TAGS");

const char* const PARED_TAGS[] = {
    "title", "table", "td", "tr", "div", "font", "span", "pre", "small", "strong",
    "i", "b",  "dl", "li","a", "h1", "h2", "h3"
};

static_assert(sizeof(PARED_TAGS)/sizeof(PARED_TAGS[0]) == N_PARED_TAGS, "expect sizeof(PARED_TAGS)/sizeof(PARED_TAGS[0]) == N_PARED_TAGS");

class TTagsForCalc {
private:
    typedef THashMap<const char*, ui32> TStringHash;
    TStringHash InterestTagsHashed;
    TStringHash ParedTagsHashed;

    void InsertStrings(TStringHash& h, const char* const arr[], size_t n) {
        for (size_t i = 0; i < n; ++i)
            h.insert(std::make_pair(arr[i], i));
    }

public:
    TTagsForCalc() {
        InsertStrings(InterestTagsHashed, INTEREST_TAGS, N_INTEREST_TAGS);
        InsertStrings(ParedTagsHashed, PARED_TAGS, N_PARED_TAGS);
    }

    int IsInterestTag(const char* tag) const {
        TStringHash::const_iterator toTag = InterestTagsHashed.find(tag);
        if (toTag != InterestTagsHashed.end())
            return toTag->second;
        else
            return -1;
    }

    int IsParedTag(const char* tag) const {
        TStringHash::const_iterator toTag = ParedTagsHashed.find(tag);
        if (toTag != ParedTagsHashed.end())
            return toTag->second;
        else
            return -1;
    }
};

static const TTagsForCalc TagsForCalc;

inline ui32 JSHash(const wchar16 *tok, size_t len) {
    ui32 hash = 1315423911;
    // todo: do we really want to ignore the last char?
    for (const wchar16 *end = tok + len - 1; tok < end; ++tok) {
        hash ^= (((hash << 5) ^ ToLower(CanonicalChar(*tok)) ^ (hash >> 2)));
    }
    return hash;
}

class TFreqWords {
private:
    TIntHash<ui32, 0x10000>::TPoolType Pool;
    TIntHash<ui32, 0x10000> Hash;
public:
    TFreqWords()
        : Pool(4096)
        , Hash(&Pool)
    {
        const char* const* ptr = FreqWordsArray;
        // loop for all words
        ui32 id = 0;
        do {
            // loop for all forms of the word
            do {
                // utf8 -> unicode
                size_t in_size = strlen(*ptr);
                size_t out_size = in_size * 2;
                TTempArray<wchar16> out(out_size);
                size_t in_readed = 0, out_writed = 0;
                RecodeToUnicode(CODES_UTF8, *ptr, out.Data(), in_size, out_size, in_readed, out_writed);
                // unicode -> hash
                ui32 hash = JSHash(out.Data(), out_writed);
                // hash -> id; do not overwrite already written value
                if (Hash.find(hash) == Hash.end())
                    Hash.insert(std::make_pair(hash, id));
            } while (*++ptr);
            id++;
        } while (*++ptr);
    }

    bool GetID(ui32 hash, ui32& id) /*const*/ {
        TIntHash<ui32, 0x10000>::iterator it = Hash.find(hash);
        if (it == Hash.end())
            return false;
        else {
            id = it->second;
            return true;
        }
    }
};

TTextFeaturesHandler::TTextFeaturesHandler()
    : HashPool(256)
    , Words((TWordsHash::TPoolType*)&HashPool)
    , Links((TLinksHash::TPoolType*)&HashPool)
    , Bigrams((TBigramsHash::TPoolType*)&HashPool)
    , TrigramsPool(256)
    , Trigrams(&TrigramsPool)
    , WordsNum(0)
    , CommentsLen(0)
    , InScript(false)
    , ScriptsLen(0)
    , SumTagsEntry(0)
    , SumWordsLen(0)
    , Tags(N_INTEREST_TAGS, 0)
    , TagsStack(N_PARED_TAGS, 0)
    , WordsInTags(N_PARED_TAGS, 0)
    , LastWords(N_GRAMS, 0)
    , FreqWords(*Singleton<TFreqWords>())
    , FreqWordsNum(0)
{
}

void TTextFeaturesHandler::SetDoc(const TString& docUrl) {
    DocUrl = docUrl;
    Host = GetHost(DocUrl);
}

void TTextFeaturesHandler::OnTokenStart(const TWideToken& wtok, const TNumerStat&)
{
    ui32 dataHash = JSHash(wtok.Token, wtok.Leng);
    SumWordsLen += wtok.Leng;

    while (LengCounts.size() <= wtok.Leng)
        LengCounts.push_back(0);
    ++LengCounts[wtok.Leng];

    TWordsHash::iterator it = Words.find(dataHash);
    if (it == Words.end())
        Words.insert( std::make_pair(dataHash, 1) );
    else
        ++it->second;

    ui32 wordFreqID;
    if (FreqWords.GetID(dataHash, wordFreqID)) {
        if (wordFreqID < FREQ_LIMIT)
            FreqWordsNum++;
        UsedFreqWords[wordFreqID] = true;
    }

    for (size_t i = 0; i < N_PARED_TAGS; ++i)
        if (TagsStack[i] > 0)
            ++WordsInTags[i];

    ++WordsNum;
    LastWords[0] = LastWords[1] * FNV_PRIME;
    LastWords[1] = LastWords[2] * FNV_PRIME;
    LastWords[2] = dataHash;
    ui32 bigramKey = LastWords[1] ^ LastWords[2];
    if (WordsNum >= 2) {
        TBigramsHash::iterator it = Bigrams.find(bigramKey);
        if (it != Bigrams.end())
            ++it->second;
        else
            Bigrams.insert(std::make_pair(bigramKey, 1));
    }
    if (WordsNum >= 3) {
        ui32 key = bigramKey ^ LastWords[0];
        TTrigramsType::iterator it = Trigrams.find(key);
        if (it != Trigrams.end())
            ++it->second.second;
        else
            Trigrams.insert(std::make_pair(key, std::make_pair(bigramKey, 1)));
    }

}

void TTextFeaturesHandler::OnZoneEntry(const TZoneEntry* zone)
{
    if (!zone || zone->NoOpeningTag || zone->IsNofollow())
        return;
    if (zone->Name && !zone->OnlyAttrs) {
        if (0 == strncmp(zone->Name, "anchor", 6)) {
            const static int A_INDEX = TagsForCalc.IsInterestTag("a");
            if (zone->IsOpen) {
                ++Tags[A_INDEX];
                ++SumTagsEntry;
            }

            const static int A_PARED_INDEX = TagsForCalc.IsParedTag("a");
            int diff = zone->IsOpen ? 1 : -1;
            TagsStack[A_PARED_INDEX] += diff;
        } else if (0 == strncmp(zone->Name, "title", 5)) {
            const static int TITLE_PARED_INDEX = TagsForCalc.IsParedTag("title");
            int diff = zone->IsOpen ? 1 : -1;
            TagsStack[TITLE_PARED_INDEX] += diff;
        }
    }

    for (size_t i = 0; i < zone->Attrs.size(); ++i) {
        const TAttrEntry& attr = zone->Attrs[i];
        if (0 == strncmp(~attr.Name, "link",4)) {
            ui32 hostHash = ComputeHash(GetHost(TStringBuf(~attr.Value, +attr.Value)));
            TLinksHash::iterator it = Links.find(hostHash);
            if (it == Links.end())
                Links.insert( std::make_pair(hostHash, 1) );
            else
                ++it->second;
        }
    }
}

void TTextFeaturesHandler::OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* ze, const TNumerStat&)
{
    if (MARKUP_IMPLIED != chunk.flags.markup) {
        const char* ptr = chunk.text;
        if (!ptr)
            return;

        size_t len = chunk.leng;
        if (len >= 4 && strncmp(ptr, "<!--", 4) == 0) {
            CommentsLen += chunk.leng;
            return;
        }

        switch (chunk.flags.type) {
            case PARSED_MARKUP: {
                if (!ze || (!ze->IsOpen && !ze->IsClose)) {
                    if (ptr[0] == '<') {
                        ++ptr;
                        --len;
                    } else {
                        break;
                    }

                    int diff = 1;

                    if (ptr[0] == '/') {
                        diff = -1;
                        if (0 == strnicmp(ptr, "/script", 7))
                            InScript = false;

                        ++ptr;
                        --len;
                    }

                    TTempBuf buf(len + 1);
                    char* tag = buf.Data();
                    memcpy(tag, ptr, len);
                    tag[len] = 0;
                    size_t end = TStringBuf{tag, len}.find_first_of(" \n\t>");
                    if (end < len)
                        tag[end] = 0;
                    else
                        break;

                    ToLower(tag, end);

                    int tagParedIndex = TagsForCalc.IsParedTag(tag);
                    if (-1 != tagParedIndex) {
                        TagsStack[tagParedIndex] += diff;
                    }

                    if (diff < 0)
                        break;

                    ++SumTagsEntry;

                    int tagInterestIndex = TagsForCalc.IsInterestTag(tag);
                    if (-1 != tagInterestIndex) {
                        ++Tags[tagInterestIndex];
                    }

                    if (strcmp(tag, "script") == 0)
                        InScript = true;
                }
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            }

            default:
                if (InScript)
                     ScriptsLen += chunk.leng;
                break;
        }
    }

    OnZoneEntry(ze);
}

void TTextFeaturesHandler::Print(TTextFeatures& textFeatures)
{
    memset(&textFeatures, 0, sizeof(float) * N_FEATURES_COUNT);
    TVector<ui32> WordsVec, TagsVec, LinksVec;
    WordsVec.resize(Words.size());
    TagsVec.reserve(N_INTEREST_TAGS);
    for (size_t i = 0; i < N_INTEREST_TAGS; ++i)
        if (Tags[i])
            TagsVec.push_back(Tags[i]);

    ui32 sumWordsEntry = 0;
    {
        size_t i = 0;
        for (TWordsHash::iterator it = Words.begin(); it != Words.end(); ++it) {
            WordsVec[i++] = it->second;
            sumWordsEntry += it->second;
        }
    }

    const static size_t IMG_INDEX = TagsForCalc.IsInterestTag("img");
    const ui32 imgsEntry = Tags[IMG_INDEX];
    const static size_t A_INDEX = TagsForCalc.IsInterestTag("a");
    const ui32 sumLinksEntry = Tags[A_INDEX];

    float linksMax = 0.f;
    for (TLinksHash::iterator it = Links.begin(); it != Links.end(); ++it) {
        if (it->second > linksMax)
            linksMax = it->second;
    }

    float trLikeliHood = 0.f, trEntropy = 0.f, trCount = WordsNum - 2;
    float biCount = WordsNum - 1;
    if (trCount < 1)
        trCount = 1;
    if (biCount < 1)
        biCount = 1;
    float trCondProb = 0.f;

    float logTrCount = log((float)trCount);
    for (TTrigramsType::iterator it = Trigrams.begin(); it != Trigrams.end(); ++it) {
        float logNum = log((float)it->second.second);
        float lg = logNum - logTrCount;
        trLikeliHood += lg;
        trEntropy += (float) it->second.second / (float) trCount * lg;
        TBigramsHash::iterator jt = Bigrams.find(it->second.first);
        lg = logNum - log((float)jt->second);
        trCondProb += lg;
    }

    trLikeliHood = -(trLikeliHood / trCount);
    trEntropy = -trEntropy;
    trCondProb = -(trCondProb / trCount);
    TrigramsProb = trLikeliHood; // similar to LikeliHood, but other remapping
    TrigramsCondProb = std::min(trCondProb, 1.0f);

    std::sort(TagsVec.begin(), TagsVec.end());
    std::sort(WordsVec.begin(), WordsVec.end());

    size_t lengMedian = 0;
    {
        ui32 pSum = 0;
        if (LengCounts.size() > 0)
            pSum = LengCounts[0];

        while ( (lengMedian < LengCounts.size()) && (2*pSum < WordsNum) ) {
            ++lengMedian;
            if (lengMedian < LengCounts.size())
                pSum += LengCounts[lengMedian];
        }
    }

    textFeatures[0] = (float) sumWordsEntry;
    textFeatures[1] = (float) WordsNum /  std::max((float) Words.size(), 1.f);
    textFeatures[2] = (float) SumWordsLen /  std::max((float) Words.size(), 1.f);
    textFeatures[3] = (float) lengMedian;
    textFeatures[4] = (TagsVec.empty()) ? 0.f : (float) TagsVec[TagsVec.size() / 2];
    textFeatures[5] = (TagsVec.empty()) ? 0.f : (float) TagsVec.back();
    textFeatures[6] = (float) imgsEntry / std::max((float) SumTagsEntry, 1.f);
    textFeatures[7] = (float) sumLinksEntry / std::max((float) SumTagsEntry, 1.f);
    textFeatures[8] = (float) SumTagsEntry / std::max((float) WordsNum, 1.f);
    textFeatures[9] = (float) sumLinksEntry;
    textFeatures[10] = (float) sumLinksEntry / std::max((float) Links.size(), 1.f);
    textFeatures[11] = (float) ScriptsLen / (float) std::max((float) (SumWordsLen + ScriptsLen + CommentsLen), 1.f);
    textFeatures[12] = (float) CommentsLen / (float) std::max((float) (SumWordsLen + ScriptsLen + CommentsLen), 1.f);
    textFeatures[13] = (float) SumWordsLen / (float) std::max((float) (SumWordsLen + ScriptsLen + CommentsLen), 1.f);
    textFeatures[14] = (float) DocUrl.length() / 255.f;
    textFeatures[15] = (float) linksMax / std::max((float)sumLinksEntry, 1.f);
    textFeatures[16] = trLikeliHood;
    textFeatures[17] = trEntropy;
    textFeatures[18] = trLikeliHood / log(std::max(float(WordsNum), 2.f));
    textFeatures[19] = trEntropy / log(std::max(float(WordsNum), 2.f));

    PercentVisibleContent = textFeatures[13];

    size_t curFeatures = N_BASE_COUNT;

    if (sumWordsEntry < 1)
        sumWordsEntry = 1;

    int curWordsNum = Words.size();
    for (size_t i = 0; i < N_MAX_WORDS; ++i) {
        curWordsNum--;
        if (curWordsNum >= 0)
            textFeatures[curFeatures + i] = (float) WordsVec[curWordsNum] /(float) sumWordsEntry;
        else
            textFeatures[curFeatures + i] = 0.f;
    }

    if (curWordsNum > 0) {
        for (size_t i = 0; i < N_AVE_WORDS; ++i) {
            int ind = (curWordsNum * (2*N_AVE_WORDS - i)/ (2*N_AVE_WORDS) - 1);
            if (ind >= 0 && ind < (int)Words.size())
                textFeatures[curFeatures + N_MAX_WORDS + i] = (float) WordsVec[ind] / (float) sumWordsEntry;
            else
                textFeatures[curFeatures + N_MAX_WORDS + i] = 0.f;
        }
    }
    curFeatures += N_MAX_WORDS + N_AVE_WORDS;

    for (size_t i = 0; i < N_INTEREST_TAGS; ++i)
        textFeatures[curFeatures + i] =  (float) Tags[i] / (float) SumTagsEntry;

    curFeatures += N_INTEREST_TAGS;

    for (size_t i = 0; i < N_PARED_TAGS; ++i) {
        textFeatures[curFeatures + i] = (float)WordsInTags[i]/std::max((float)WordsNum, 1.f);
    }
}

void TTextFeaturesHandler::OnTextEnd(const IParsedDocProperties*, const TNumerStat& ) {
    TTextFeatures f;
    Print(f);

    for (size_t i = 0; i < N_FEATURES_COUNT; ++i) {
        f[i] = (f[i] - MIN_VALS[i]) / (MAX_VALS[i] - MIN_VALS[i]);
        f[i] = std::min(1.f, f[i]);
        f[i] = std::max(0.f, f[i]);
    }
    TextVal = Eval(f);
    TextVal = (TextVal - MIN_TR_VAL) / (MAX_TR_VAL - MIN_TR_VAL);
    TextVal = std::min(1.f, TextVal);
    TextVal = std::max(0.f, TextVal);
    TextVal = remapTR.Remap(TextVal);

    LikeliHood = remapLH.Remap(f[16]);
    LikeliHood = std::min(1.f, LikeliHood);
    LikeliHood = std::max(0.f, LikeliHood);
}

float TTextFeaturesHandler::GetTextVal() const {
    return TextVal;
}

float TTextFeaturesHandler::GetLikeliHood() const {
    return LikeliHood;
}

void TTextFeaturesHandler::InsertFactors(IDocumentDataInserter& inserter) const {
    inserter.StoreErfDocAttr("TextF", Sprintf("%f", GetTextVal()));
    inserter.StoreErfDocAttr("TextL", Sprintf("%f", GetLikeliHood()));
    int sourceVal = GetNumWordsInText();
    float factorVal = sourceVal / (400.0f + sourceVal);
    inserter.StoreErfDocAttr("RusWordsInText", Sprintf("%f", factorVal));
    sourceVal = GetNumWordsInTitle();
    factorVal = sourceVal / (5.0f + sourceVal);
    inserter.StoreErfDocAttr("RusWordsInTitle", Sprintf("%f", factorVal));
    sourceVal = GetMeanWordLength();
    factorVal = sourceVal / (40.0f + sourceVal);
    inserter.StoreErfDocAttr("MeanWordLength", Sprintf("%f", factorVal));
    factorVal = GetPercentWordsInLinks();
    factorVal = factorVal / (0.1f + 0.9f * factorVal);
    inserter.StoreErfDocAttr("PercentWordsInLinks", Sprintf("%f", factorVal));
    factorVal = GetPercentVisibleContent();
    factorVal = factorVal / (0.05f + 0.95f * factorVal);
    inserter.StoreErfDocAttr("PercentVisibleContent", Sprintf("%f", factorVal));
    inserter.StoreErfDocAttr("PercentFreqWords", Sprintf("%f", GetPercentFreqWords()));
    inserter.StoreErfDocAttr("PercentUsedFreqWords", Sprintf("%f", GetPercentUsedFreqWords()));
    factorVal = GetTrigramsProb();
    factorVal = factorVal / (40.0 + factorVal);
    inserter.StoreErfDocAttr("TrigramsProb", Sprintf("%f", factorVal));
    factorVal = GetTrigramsCondProb();
    factorVal = factorVal / (0.01 + 0.99 * factorVal);
    inserter.StoreErfDocAttr("TrigramsCondProb", Sprintf("%f", factorVal));
}
