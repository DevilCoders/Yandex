#include "synnorm.h"

#include <kernel/synnorm/synset.pb.h>

#include <kernel/gazetteer/richtree/gztres.h>

#include <kernel/qtree/richrequest/lemmas.h>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/reqerror/reqerror.h>

#include <kernel/search_daemon_iface/reqtypes.h>

#include <kernel/lemmer/core/wordinstance.h>
#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/stopwords/stopwords.h>

#include <util/string/split.h>
#include <util/stream/file.h>
#include <util/generic/algorithm.h>
#include <util/string/vector.h>
#include <util/charset/wide.h>
#include <util/memory/blob.h>

namespace NSynNorm {

// TStopwordChecker incapsulates knowledge about minor words that can be excluded from query.
// In fact words are stolen from doppelgangers normalization.

const char* STOPWORDS[] =
{
    "a",
    "the",
    "and",
    "an",
    "и",
    "что",
    "такое",
    "в",
    "во",
    "на",
    "который",
    "которая",
    "которое",
    "а",
    "у",
    "о",
    "к",
    "об",
    "ко",
    "для",
    "такой",
    "чтобы",
    "чтоб",
    "на",
    "про",
    "с",
    "со",
    "of",
    nullptr
};

const char* LARGE_INTENTS[] =
{
    "скачать",
    "бесплатно",
    "сайт",
    "это",
    nullptr
};

const char* SOME_ABBREVS[] =
{
    "гру",
    "ати",
    nullptr
};

class TStopwordChecker {
public:
    TStopwordChecker(const TWordFilter* wf = nullptr)
        : WordFilter(wf)
    {
        AddStopwords(STOPWORDS);
        AddStopwords(LARGE_INTENTS);
        AddStopwords(SOME_ABBREVS);
    }

    bool IsStopword(const TUtf16String& word, const TLangMask& lang, EStickySide side) const {
        if (WordFilter && WordFilter->IsStopWord(word, lang, &side)) {
            return true;
        }
        return Stopwords_.find(word) != Stopwords_.end();
    }

private:
    void AddStopwords(const char* list []) {
        for (const char** w = list; *w; ++w) {
            Stopwords_.insert(UTF8ToWide(*w));
        }
    }

private:
    THashSet<TUtf16String> Stopwords_;
    const TWordFilter* WordFilter;
};

typedef TVector<const TLemmaForms*> TLemmaPointers;

// A helper class to word with sorted gazetteer results.

class TSortedSynsets {
private:
    static inline bool CompareWordCount(const TGztResultItem* lft, const TGztResultItem* rgh) {
        return lft->Size() > rgh->Size();
    }

public:
    void Reset(const TGztResults& results)
    {
        Sorted_.clear();
        for (TGztResults::TIndexIterator iter = results.FindByExactType(TMember::descriptor());
             iter.Ok();
             ++iter)
        {
            Sorted_.push_back(&*iter);
        }
        // Sort in order of decreasing word count.
        Sort(Sorted_.begin(), Sorted_.end(), CompareWordCount);

        Synsets_.resize(Size());
        for (size_t i = 0; i < Size(); i++) {
            Synsets_[i] = Sorted_[i]->IterRefs(TMember::kSynsetFieldNumber).LoadArticle();
        }
    }

    size_t Size() const {
        return Sorted_.size();
    }

    const TGztResultItem& GetGztResult(size_t i) const {
        return *Sorted_[i];
    }

    const TSynset* GetSynset(size_t i) const {
        return Synsets_[i].As<TSynset>();
    }

private:
    TVector<const TGztResultItem*> Sorted_;
    TVector<TArticlePtr> Synsets_;
};

// A helper to extract lemmas from rich tree.

class TGetNonBastardLemmas : public TLemmaProcessor {
public:
    TGetNonBastardLemmas(TLemmaPointers& dest)
        : Dest_(dest)
        , StopwordFound_(false)
    {
    }

    void OnLemmerWordNode(const TWordNode& node, bool& /*stop*/) override {
        for (TWordNode::TCLemIterator lemmaIter = node.LemsBegin();
             lemmaIter != node.LemsEnd();
             ++lemmaIter)
        {
            if (!lemmaIter->IsBastard()) {
                Dest_.push_back(lemmaIter);
            }
            StopwordFound_ |= lemmaIter->IsStopWord();
        }
    }

    void OnNonLemmerWordNode(const TWordNode& /*node*/, bool& /*stop*/) override {
    }

    bool StopwordFound() const {
        return StopwordFound_;
    }

private:
    TLemmaPointers& Dest_;
    bool StopwordFound_;
};

// Aggregates information about request word.
// NOT thread-safe.

class TWordSummary {
public:
    TWordSummary(const TRichRequestNode* wordRoot)
        : IsStopword(false)
        , IsSecondOrLaterWordOfSynset(false)
        , Synset_(nullptr)
        , MatchedLemmaCount_(0)
    {
        Original = wordRoot->WordInfo->GetNormalizedForm();
        TGetNonBastardLemmas lemmaCollector(Lemmas_);
        lemmaCollector.CollectLemmas(wordRoot);
        IsStopword = lemmaCollector.StopwordFound();
    }

    void MarkSpecificStopwords(const TStopwordChecker& checker) {
        for (const auto& lemma : Lemmas_) {
            if (checker.IsStopword(lemma->GetLemma(), lemma->GetLanguage(), lemma->GetStickiness())) {
                IsStopword = true;
            }
        }
    }

    const TSynset* GetSynset() const {
        // <MatchedLemmaCount_> can be more than <Lemmas_.size()> in a special case
        // when word belongs to some collocation, part of synset S,
        // and itself is a part of synset S. That's why >= is used.
        return MatchedLemmaCount_ >= Lemmas_.size() ? Synset_ : nullptr;
    }

    void AddSynsetForOneLemma(const TSynset* synset) {
        if (Synset_ == nullptr) {
            Synset_ = synset;
        }
        // Ignore all synsets except first met.
        if (Synset_->GetRepr() != synset->GetRepr()) {
            return;
        }
        MatchedLemmaCount_++;
    }

    void SetSynsetForAllLemmas(const TSynset* synset) {
        Synset_ = synset;
        MatchedLemmaCount_ = Lemmas_.size();
    }

    TUtf16String GetResponsePart() const {
        if (IsStopword || IsSecondOrLaterWordOfSynset) {
            return TUtf16String();
        }
        if (GetSynset()) {
            return UTF8ToWide(GetSynset()->GetRepr());
        }
        if (Lemmas_.size() && HasSingleLemma()) {
            return Lemmas_.back()->GetLemma();
        }
        return TUtf16String(Original);
    }

    TString DebugString() const {
        TVector<TString> lemmaStrings;
        for (size_t i = 0; i < Lemmas_.size(); i++) {
            lemmaStrings.push_back(WideToUTF8(Lemmas_[i]->GetLemma()));
        }
        return Sprintf("(%s;%s;sw=%d;ss=%s)",
                       WideToUTF8(Original).data(),
                       JoinStrings(lemmaStrings, "|").data(),
                       static_cast<int>(IsStopword),
                       GetSynset() ? GetSynset()->GetRepr().data() : "");
    }

private:
    bool HasSingleLemma() const {
        for (size_t i = 0; i + 1 < Lemmas_.size(); i++) {
            if (Lemmas_[i]->GetLemma() != Lemmas_[i + 1]->GetLemma()) {
                return false;
            }
        }
        return true;
    }

public:
    bool IsStopword;
    bool IsSecondOrLaterWordOfSynset;
    TWtringBuf Original;

private:
    TLemmaPointers Lemmas_;

    const TSynset* Synset_;
    size_t MatchedLemmaCount_;
};

// Aggregate all essential for normalization information about request.

class TRequestSummary {
public:
    TRequestSummary() = default;

    void Clear() {
        Words_.clear();
    }

    void Reset(TRichNodePtr request) {
        Clear();
        for (TUserWordsIterator wordIter(request); !wordIter.IsDone(); ++wordIter) {
            Words_.push_back(TWordSummary(&*wordIter));
        }
    }

    void MarkSpecificStopwords(const TStopwordChecker& checker) {
        for (size_t i = 0; i < Words_.size(); i++) {
            Words_[i].MarkSpecificStopwords(checker);
        }
    }

    void MarkSynsets(const TSortedSynsets& sortedSynsets) {
        for (size_t i = 0; i < sortedSynsets.Size(); i++) {
            const TGztResultItem& res = sortedSynsets.GetGztResult(i);
            const TSynset* synset = sortedSynsets.GetSynset(i);
            size_t start = res.GetOriginalStartIndex();
            size_t stop = res.GetOriginalStopIndex();
            Y_ASSERT(stop <= Words_.size());

            // All multiword keys are EXACT_MATCH.
            // We trust them.
            if (start + 1 < stop) {
                if (AllWordsFree(start, stop)) {
                    SetSynsetForAllLemmas(start, stop, synset);
                }
            }
            else {
                // Each lemma of word should correspond to the same synset.
                // Each lemma gives one match in gazetteer.
                // So logic is following: count number of matches of the synset
                // for word and compare it with lemma count.
                Words_[start].AddSynsetForOneLemma(synset);
            }
        }
    }

    void ConstructResponse(TVector<TUtf16String>& response) const {
        for (size_t i = 0; i < Words_.size(); i++) {
            TUtf16String part = Words_[i].GetResponsePart();
            if (!part.empty()) {
                response.push_back(part);
            }
        }
    }

    TString DebugString() const {
        TVector<TString> wordStrings(Words_.size());
        for (size_t i = 0; i < Words_.size(); i++) {
            wordStrings[i] = Words_[i].DebugString();
        }
        return JoinStrings(wordStrings, "&");
    }

private:
    bool AllWordsFree(size_t start, size_t stop) {
        for (size_t i = start; i < stop; i++) {
            if (Words_[i].GetSynset() != nullptr) {
                return false;
            }
        }
        return true;
    }

    void SetSynsetForAllLemmas(size_t start, size_t stop, const TSynset* synset) {
        for (size_t i = start; i < stop; i++) {
            Words_[i].SetSynsetForAllLemmas(synset);
            if (i > start) {
                Words_[i].IsSecondOrLaterWordOfSynset = true;
            }
        }
    }

private:
    TVector<TWordSummary> Words_;
};

// An inner class that implements all functionality of TSynNormalizer.

class TSynNormalizer::TImpl {
public:
    TImpl()
        : StopwordChecker_(new TStopwordChecker())
    {}

    int operator& (IBinSaver& saver) {
        saver.Add(1, &SynsetsData_);
        saver.Add(1, &StopwordsData_);
        if (!saver.IsReading())
           return 0;
        if (SynsetsData_.Size()) {
            ResetGazetteer();
        }
        if (StopwordsData_.Size()) {
            ResetStopwords();
        }
        return 0;
    }

    void LoadSynsets(const TBlob& gztBinData) {
        SynsetsData_ = gztBinData;
        ResetGazetteer();
    }

    void LoadStopwords(const TBlob& stopwordsData) {
        StopwordsData_ = stopwordsData;
        ResetStopwords();
    }

    template <bool ReturnDebugString>
    TUtf16String Normalize(const TUtf16String& query, const TLangMask& langMask, bool sortWords) const
    {
        try {
            Y_ASSERT(Synsets_.Get());


            TCreateTreeOptions options(TLanguageContext(langMask, nullptr));
            options.Reqflags |= RPF_USE_TOKEN_CLASSIFIER;

            TRichTreePtr richTree = CreateRichTree(
                query,
                options
            );

            TGztResults gztResults(richTree->Root, Synsets_.Get());

            return Normalize<ReturnDebugString>(richTree->Root, gztResults, sortWords);
        } catch (TError e) {
            if (e.GetCode() == yxREQ_EMPTY || e.GetCode() == yxSYNTAX_ERROR) {
                return TUtf16String();
            }
            throw e;
        }
    }

    template <bool ReturnDebugString>
    TUtf16String Normalize(TRichNodePtr root, const TGztResults& gztResults, bool sortWords) const {
        TRequestSummary request;
        request.Reset(root);
        request.MarkSpecificStopwords(*StopwordChecker_);

        TSortedSynsets sortedSynsets;
        sortedSynsets.Reset(gztResults);

        request.MarkSynsets(sortedSynsets);

        return
            ReturnDebugString
            ? UTF8ToWide(request.DebugString())
            : ConstructResponse(request, sortWords);
    }

private:
    void ResetGazetteer() {
        Synsets_.Reset(new TGazetteer(SynsetsData_));
    }

    void ResetStopwords() {
        TMemoryInput input(StopwordsData_.Begin(), StopwordsData_.Size());
        Stopwords_.InitStopWordsList(input);
        StopwordChecker_.Reset(new TStopwordChecker(&Stopwords_));
    }

    void LoadFileToVectorChar(const TString& path, TVector<char>& data) {
        TFile file(path, OpenExisting | RdOnly);
        data.resize(file.GetLength());
        file.Read(data.begin(), data.size());
    }

    TUtf16String ConstructResponse(const TRequestSummary& request, bool sortWords) const {
        TVector<TUtf16String> resultWords;
        request.ConstructResponse(resultWords);

        if (sortWords) {
            Sort(resultWords.begin(), resultWords.end());
        }

        // Better join in arcadia?
        return JoinStrings(resultWords.begin(), resultWords.end(), (const wchar16*)(L" "));
    }

private:
    THolder<const TGazetteer> Synsets_;
    TBlob SynsetsData_;

    TWordFilter Stopwords_;
    TBlob StopwordsData_;

    THolder<const TStopwordChecker> StopwordChecker_;
};

// TSynNormalizer pretty straight-forward implementation.

TSynNormalizer::TSynNormalizer()
    : Impl_(new TImpl)
{
}

TSynNormalizer::~TSynNormalizer() {
}

int TSynNormalizer::operator&(IBinSaver& saver) {
    saver.Add(1, Impl_.Get());
    return 0;
}

void TSynNormalizer::LoadSynsets(const TString& gztBinPath) {
    Impl_->LoadSynsets(TBlob::FromFile(gztBinPath));
}

void TSynNormalizer::LoadSynsets(const TBlob& gztData) {
    Impl_->LoadSynsets(gztData);
}

void TSynNormalizer::LoadStopwords(const TString& stopwordsPath) {
    Impl_->LoadStopwords(TBlob::FromFile(stopwordsPath));
}

void TSynNormalizer::LoadStopwords(const TBlob& stopwordsData) {
    Impl_->LoadStopwords(stopwordsData);
}

TString TSynNormalizer::NormalizeUTF8(const TString& query, const TLangMask& langMask, bool sortWords) const
{
    return WideToUTF8(Impl_->Normalize<false>(UTF8ToWide(query), langMask, sortWords));
}

TUtf16String TSynNormalizer::Normalize(const TUtf16String& query, const TLangMask& langMask, bool sortWords) const
{
    return Impl_->Normalize<false>(query, langMask, sortWords);
}

TUtf16String TSynNormalizer::Normalize(TRichNodePtr root, const TGztResults& gztResults, bool sortWords) const
{
    return Impl_->Normalize<false>(root, gztResults, sortWords);
}

TString TSynNormalizer::DebugRequestSummaryString(const TString& query, const TLangMask& langMask, bool sortWords) const
{
    return WideToUTF8(Impl_->Normalize<true>(UTF8ToWide(query), langMask, sortWords));
}

TString NormalizeWildcardsUsingSynnorm(const TStringBuf& synnormRequest)
{
    TUtf16String synnormRequestW;
    try {
        synnormRequestW = UTF8ToWide(synnormRequest);
        return WideToUTF8(NormalizeWildcardsUsingSynnorm(synnormRequestW));
    } catch (yexception&) { // e.g. exotic UTF-8 symbols (out of Basic Multilingual Plane)
        return TString();
    }
}

TUtf16String NormalizeWildcardsUsingSynnorm(const TUtf16String& synnormRequest)
{
    static const size_t MaxSearchWordLen = 5;

    TVector<TWtringBuf> words;
    StringSplitter(synnormRequest).Split(u' ').AddTo(&words);

    bool changed = false;
    for (size_t n = 0; n < words.size(); ++n) {
        if (words[n].size() > MaxSearchWordLen) {
            words[n] = words[n].substr(0, MaxSearchWordLen);
            changed = true;
        }
    }

    TUtf16String ret;

    if (changed) {
        for (size_t n = 0; n < words.size(); ++n) {
            if (n) {
                ret.push_back(' ');
            }
            ret += words[n];
        }
    } else {
        ret = synnormRequest;
    }

    return ret;
}

};
