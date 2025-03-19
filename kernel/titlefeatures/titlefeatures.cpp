#include <kernel/search_types/search_types.h>
#include "titlefeatures.h"

#include <ysite/relevance_tools/dict.h>
#include <ysite/relevance_tools/subphrase_finder_file.h>
#include <kernel/idf/idf.h>
#include <kernel/indexer/face/inserter.h>
#include <ysite/yandex/pure/pure.h>

#include <library/cpp/on_disk/2d_array/array2d_writer.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/vector.h>

static constexpr ui8 maxTitleLen = 30;

struct TTitleWordInfo {
    ui32 ExactHits;
    double Weight;

    TTitleWordInfo()
        : ExactHits(0)
        , Weight(1.0)
    {
    }
};

static float CalcBM25(const TVector<TTitleWordInfo>& wordInfos, ui32 docLen, float avDocLen = 1400.0)
{
    if (docLen == 0 || !(avDocLen > 0)) {
        return 0.0;
    }
    float term = float(0.5 + 1.5 * (float)docLen / avDocLen);
    float result = 0.0f;
    for (size_t i = 0; i < wordInfos.size(); ++i) {
        float tf = (float)wordInfos[i].ExactHits / (float)docLen;
        result += (float)wordInfos[i].Weight * (3.0f * tf) / (tf + term);
    }
    return result;
}

static inline float NormFeature(float value, float param = 1.0)
{
    return value / (value + param);
}

/////////////////////////
// For prewalrus
/////////////////////////

class TTitleTracer {
private:
    typedef THashMap<const char*, TVector<size_t> > TTitleDict;
    TTitleDict ExactForms;
    const ui64 PureCollectionLength;
    TVector<TTitleWordInfo> WordInfos;
    ui32 DocLen;
    ui32 NumHits;

public:
    TTitleTracer(const ui64 pureCollectionLength)
        : ExactForms(0)
        , PureCollectionLength(pureCollectionLength)
        , WordInfos(0)
        , DocLen(0)
        , NumHits(0)
    {
    }

    void Process(const NIndexerCore::TDirectTextData2& directText);

    ui32 GetDocLen() const
    {
        return DocLen;
    }

    const TVector<TTitleWordInfo>& GetWordInfos() const
    {
        return WordInfos;
    }

    size_t GetTitleLen() const
    {
        return WordInfos.size();
    }

    ui32 GetNumHits() const
    {
        return NumHits;
    }
};

void TTitleTracer::Process(const NIndexerCore::TDirectTextData2& directText) {
    typedef TVector<const NIndexerCore::TLemmatizedToken*> TTokens;
    const TTokens noTokens;
    typedef THashMap<const char*, TTokens> TForms;
    TForms forms;
    for (size_t i = 0; i < directText.EntryCount; ++i) {
        const NIndexerCore::TDirectTextEntry2& e = directText.Entries[i];
        if (!e.Token)
            continue;

        if (TWordPosition::GetRelevLevel(e.Posting) == BEST_RELEV) {
            // forms with different languages considered as the same
            for (size_t j = 0; j < e.LemmatizedTokenCount; ++j) {
                const NIndexerCore::TLemmatizedToken* token = e.LemmatizedToken + j;
                std::pair<TForms::iterator, bool> res = forms.insert(std::make_pair(token->FormaText, noTokens));
                res.first->second.push_back(token);
            }

            for (TForms::const_iterator f = forms.begin(); f != forms.end(); ++f) {
                DocLen += 1;
                if (WordInfos.size() >= maxTitleLen)
                    continue;
                WordInfos.push_back(TTitleWordInfo());
                const size_t wordIndex = WordInfos.size() - 1;
                ExactForms[f->first].push_back(wordIndex);
                i64 maxTermCount = 0;
                for (size_t lemmIdx = 0; lemmIdx < f->second.size(); ++lemmIdx) {
                    const NIndexerCore::TLemmatizedToken* token = f->second[lemmIdx];
                    if (maxTermCount < token->TermCount)
                        maxTermCount = token->TermCount;
                }
                WordInfos[wordIndex].Weight = TermCount2Idf(PureCollectionLength, maxTermCount);
            }
        } else {
            for (size_t j = 0; j < e.LemmatizedTokenCount; ++j) {
                const NIndexerCore::TLemmatizedToken* token = e.LemmatizedToken + j;
                forms.insert(std::make_pair(token->FormaText, noTokens));
            }

            for (TForms::const_iterator f = forms.begin(); f != forms.end(); ++f) {
                DocLen += 1;
                TTitleDict::const_iterator it = ExactForms.find(f->first);
                if (it != ExactForms.end()) {
                    for (TVector<size_t>::const_iterator wordIter = it->second.begin(); wordIter != it->second.end(); ++wordIter) {
                        WordInfos[*wordIter].ExactHits += 1;
                        NumHits += 1;
                    }
                }
            }
        }

        forms.clear();
    }
}

TTitleFeaturesProcessor::TTitleFeaturesProcessor(const TTitleFeaturesConfig& config)
{
    TPure pure(config.PureTrieFile);
    PureCollectionLength = pure.GetCollectionLength();
}

void TTitleFeaturesProcessor::ProcessDirectText2(IDocumentDataInserter* inserter, const NIndexerCore::TDirectTextData2& directText, ui32 /*docId*/)
{
    TTitleTracer titleTracer(PureCollectionLength);
    titleTracer.Process(directText);

    float bm25ex = CalcBM25(titleTracer.GetWordInfos(), titleTracer.GetDocLen());
    inserter->StoreErfDocAttr("TitleBM25Ex", Sprintf("%.3g", NormFeature((titleTracer.GetTitleLen() > 0) ? bm25ex * float(titleTracer.GetNumHits()) / float(titleTracer.GetTitleLen()) : 0.0f, 5.0f)));
}

/////////////////////////
// For erfcreate
/////////////////////////

TTitleLRBM25Calculator::TTitleLRBM25Calculator()
    : WordInfos(0)
    , ExactForms(0)
    , DocLen(0)
    , NumHits(0)
{
}

TTitleLRBM25Calculator::~TTitleLRBM25Calculator()
{
}

void TTitleLRBM25Calculator::SetTitle(const TString& title)
{
    TVector<TString> words;
    StringSplitter(title).Split(' ').SkipEmpty().Limit(maxTitleLen).Collect(&words);
    for (size_t i = 0; i < words.size(); ++i) {
        AddTitleWord(words[i]);
    }
}

void TTitleLRBM25Calculator::Feed(const TString& text)
{
    TStringStream buffer;
    static const CodePage* codesYandex = CodePageByCharset(CODES_YANDEX);
    for (TString::const_iterator it = text.begin(); it != text.end(); ++it) {
        if (codesYandex->IsAlnum(*it)) {
            buffer << codesYandex->ToLower(*it);
        } else if (codesYandex->IsSpace(*it)) {
            if (!buffer.Empty()) {
                DetectHits(buffer.Str());
            }
            buffer.clear();
        }
    }
    if (!buffer.Empty()) {
        DetectHits(buffer.Str());
    }
}

float TTitleLRBM25Calculator::GetFactor() const
{
    float bm25 = CalcBM25(WordInfos, DocLen, 460.0);
    float titleLen = (float)WordInfos.size();
    return float((NumHits > 0) ? 1.0 / ( (bm25 * titleLen / (float)NumHits) + 1.0) : 0.0);
}

void TTitleLRBM25Calculator::AddTitleWord(const TString& word)
{
    WordInfos.push_back(TTitleWordInfo());
    size_t currentWordIndex = WordInfos.size() - 1;
    ExactForms[word].push_back(currentWordIndex);
}

void TTitleLRBM25Calculator::DetectHits(const TString& word)
{
    ++DocLen;
    TTitleDict::const_iterator dictIter = ExactForms.find(word);
    if (dictIter != ExactForms.end()) {
        for (TVector<size_t>::const_iterator wordIter = dictIter->second.begin(); wordIter != dictIter->second.end(); ++wordIter) {
            ++NumHits;
            WordInfos[*wordIter].ExactHits += 1;
        }
    }
}
