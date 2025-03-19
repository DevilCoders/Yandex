#pragma once

#include "supernode.h"

#include <kernel/gazetteer/gazetteer.h>

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/tokenlist.h>

#include <library/cpp/deprecated/iter/begin_end.h>
#include <library/cpp/deprecated/iter/mapped.h>
#include <library/cpp/deprecated/iter/vector.h>
#include <library/cpp/deprecated/iter/ranged_for.h>

#include <util/generic/noncopyable.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

// Defined in this file
class TGztResultItem;
class TGztResultPosition;
class TGztResults;
struct TGztResultData;

// A single found article of gazetteer.
class TGztResultItem: public NGzt::TArticlePtr, TNonCopyable {
public:
    inline size_t Size() const {
        return StopIndex - StartIndex;
    }

    // Pointer to original phrase (sequence of nodes).
    // Note that it reflects only initial state of the query tree without any modification made by wizard-rules.
    // So, for example, it could contain nodes, dropped from current state of tree.
    inline const TVector<TRichNodePtr>& GetOriginalPhrase() const {
        return Phrase.RichNodes();
    }

    // These two indexes specify position of this gzt-result in original query tree.
    // They are valid only for GetOriginalPhrase()
    inline size_t GetOriginalStartIndex() const {
        return StartIndex;
    }

    inline size_t GetOriginalStopIndex() const {
        return StopIndex;
    }

    inline bool operator<(const TGztResultItem& a) const {
        if (StartIndex != a.StartIndex)
            return StartIndex < a.StartIndex;
        else if (StopIndex != a.StopIndex)
            return StopIndex < a.StopIndex;
        else
            return GetTitle() < a.GetTitle();
    }

    inline bool operator==(const TGztResultItem& a) const {
        return a.StartIndex == StartIndex
            && a.StopIndex == StopIndex
            && a.GetId() == GetId();
    }

    inline const NGzt::TMessage* GetArticle() const {
        return Get();
    }

    // True if this result was added artificially by calling TGztResults::AddArtificial()
    inline bool IsArtificial() const {
        return !IsFromPool();
    }

    // True if this result covers all word nodes of the request
    inline bool IsWholeRequest() const {
        return StartIndex == 0 && StopIndex == Phrase.Size();
    }

    // Check if this result is valid for specified tree, i.e. all its phrase nodes can be found
    // among specified tree sub-nodes (of any level) in same adjacent sequence without "holes".
    bool IsValidForTree(const TRichRequestNode& treeRoot) const;

    typedef NIter::TBeginEndIterator<TRichRequestNode::TNodesVector::const_iterator, const TRichNodePtr> TNodeIterator;
    inline TNodeIterator IterNodes() const {
        return TNodeIterator(Phrase.RichNodes().begin() + StartIndex,
                             Phrase.RichNodes().begin() + StopIndex);
    }

    const TRichRequestNode& FirstNode() const {
        return *GetOriginalPhrase()[StartIndex];
    }

    const TRichRequestNode& LastNode() const {
        Y_ASSERT(StopIndex > 0);
        return *GetOriginalPhrase()[StopIndex - 1];
    }

    // Return coords of gzt item in terms of specified @tokens, or false
    bool MapToTokens(const TTokenList& tokens, TTokenList::TSpan& span) const {
        return tokens.Map(Phrase.RichNodes().begin() + StartIndex,
                          Phrase.RichNodes().begin() + StopIndex, span);
    }

    // "static" representation of a word where this gzt-article was found
    // (i.e. not bound to gzt iterations)
    struct TWord {
        TUtf16String Text;
        bool IsLemma;

        inline TWord(const TUtf16String& text, bool isLemma)
            : Text(text), IsLemma(isLemma)
        {
        }

        // Returns true if given @node corresponds to @this word (i.e. have same node ID and lemma text)
        bool WasFoundOn(const TRichRequestNode& node) const;
    };

    const TVector<TWord>& GetWords() const {
        return Words;
    }

    // Prints a text corresponding to key of gzt article which was found.
    // Words a separated with space and exact-form-found words are marked with '!'
    // A @buffer string can be re-used to avoid allocating new buffer.
    TUtf16String RestoreFoundKey(TUtf16String* buffer = nullptr) const;

    TUtf16String DebugString() const;

private:
    friend class TGztResults;
    friend struct TNodeCollection;

    // only TGztResults can construct these items
    TGztResultItem(const NGzt::TSuperNodeVector& nodes, const NGzt::TArticlePtr& art, size_t begin, size_t end);

    template <typename TInput>
    void ResetWords(const NGzt::TArticleIter<TInput>& iter);
    void ResetWords(const NGzt::TSuperNodeVector& nodes, size_t begin, size_t end);

    // Checks if @checkedNode corresponds to Phrase[StartIndex + @myNodeIndex] (i.e. they have same node id and same word text)
    bool CompareNodes(size_t myNodeIndex, const TRichRequestNode& checkedNode) const;
    bool TrySkip(size_t& myNodeIndex, const TRichRequestNode& node) const;

    bool FindLongestSubPhrase(const TRichRequestNode& node, size_t& curIndex) const;

private:
    // The text of words used to find this gzt-result.
    // This will be an original word if it was found as EXACT_FORM.
    // This will be a lemma of original word it is was found as ALL_FORMS.
    TVector<TWord> Words;

    const NGzt::TSuperNodeVector& Phrase;    // Underlying phrase nodes (all leaves of a phrase)

    // Indexes from @Phrase corresponding to this gzt-result beginning and ending: Phrase[StartIndex:StopIndex)
    size_t StartIndex, StopIndex;
};

// Wrapper for TGztResultItem with additional info about its relative position in given nodes
// This class objects are returned as result of TGztResults::FindByContentNodeXXX(...) methods calls.
class TGztResultPosition
{
public:
    inline TGztResultPosition(const TGztResultItem* item, size_t pos)
        : Item(item), Pos(pos)
    {
    }

    TGztResultPosition(const TGztResultPosition&) = default;
    TGztResultPosition(TGztResultPosition&&) = default;
    TGztResultPosition& operator=(const TGztResultPosition&) = default;
    TGztResultPosition& operator=(TGztResultPosition&&) = default;

    inline size_t Size() const {
        return Item->Size();
    }

    inline size_t GetStartIndex() const {
        return Pos;
    }

    inline size_t GetStopIndex() const {
        return Pos + Size();
    }

    inline size_t GetLastIndex() const {
        return Pos + Size() - 1;
    }

    bool Contains(size_t pos) const {
        // position is stored as [startIndex, stopIndex)
        return GetStartIndex() <= pos && GetStopIndex() > pos;
    }

    bool IsInside(size_t first, size_t second) const {
        // both gzt position and test range are stored as [startIndex, stopIndex)
        return GetStartIndex() >= first && GetStopIndex() <= second;
    }

    bool IsInside(const std::pair<size_t, size_t>& pos) const {
        return IsInside(pos.first, pos.second);
    }

    inline const TGztResultItem& operator*() const {
        return *Item;
    }

    inline const TGztResultItem* operator->() const {
        return Item;
    }

private:
    const TGztResultItem* Item;
    size_t Pos;
};

// Uses gazetteer to find all occurrences of gzt-article in specified @input.
// Collects all found articles and make several mappings for quick access:
//   descriptor -> found articles of strictly this type.
//   descriptor -> found articles of this type and of all derived types.
//   name -> article with this name (title).
class TGztResults : TNonCopyable
{
public:
    TGztResults(const TGazetteer* gazetteer, size_t itemsLimit = 0);
    TGztResults(TRichNodePtr inputTree, const TGazetteer* gazetteer, size_t itemsLimit = 0);
    ~TGztResults();

    // If @normalize is true, also find articles that would match if kernel/lemmer's normalization
    // was applied to each text node in the tree.
    void Reset(TRichNodePtr inputTree, bool normalize = false);

    // Search on @inputTree with extra-forms collected from node's lemma forms
    void ResetWithExtraForms(TRichNodePtr inputTree);

    // Do gzt-search on current Nodes using specified @gzt
    // and add all found articles as artificial results to this results.
    void AddFromGzt(const TGazetteer& gazetteer, size_t itemsLimit = 0);


    void Clear();

    size_t Size() const;
    const TGztResultItem& operator[](size_t index) const;
    const TGazetteer* Gazetteer() const;

    typedef TGztResultItem value_type;
    typedef NIter::TRangedForIteratorAdapter<TGztResults> TRangedForGztResultsIterator;
    TRangedForGztResultsIterator begin() const;
    TRangedForGztResultsIterator end() const;

    // Iterator over gzt-results (using *this as a mapper from index to item)
    typedef NIter::TMappedIterator<const TGztResultItem, NIter::TVectorIterator<const size_t>, TGztResults> TIndexIterator;
    friend class NIter::TMappedIterator<const TGztResultItem, NIter::TVectorIterator<const size_t>, TGztResults>;

    // Find gzt-results by features
    TIndexIterator FindByExactType(const NGzt::TDescriptor* type) const;
    TIndexIterator FindByBaseType(const NGzt::TDescriptor* type) const;
    const TGztResultItem* FindByTitle(const TUtf16String& title) const;

    // Gzt-results found on only specified nodes (or on any sub-segment of this nodes):

    // for TConstNodesVector
    bool FindByContentNodes(const TConstNodesVector& nodes, TVector<TGztResultPosition>& results) const;
    bool FindByContentNodes(const TVector<TRichRequestNode*>& nodes, TVector<TGztResultPosition>& results) const;

    // for TNodesVector
    bool FindByContentNodes(const TRichRequestNode::TNodesVector& nodes, TVector<TGztResultPosition>& results) const;

    // for standard rich tree iterator (derived from TBaseRichNodeIterator)
    template <class TRichTreeIterator>
    bool FindByContentNodeIterator(TRichTreeIterator& nodes, TVector<TGztResultPosition>& results) const;

    // for gzt-specific nodes iterators (iterating over "const TRichRequestNode&")
    template <class TGztNodeIterator>
    bool FindByContentNodeGztIterator(TGztNodeIterator& nodes, TVector<TGztResultPosition>& results) const;


    void SelectBestCoverage(TVector<TGztResultPosition>& results) const;





    // Associate an artificial (non-gazetteer) article with request segment
    // Segment is specified by its @first and @last node, plus a common @parent node (at any level, not necessarily immediate parent)
    // In case of single-word segment all node refs could be the same.
    // Returns false if it cannot map specified segment into node indexes.
    bool AddArtificial(const NGzt::TMessage& article, const TRichRequestNode& first, const TRichRequestNode& last, const TRichRequestNode& parent, const TUtf16String& title = TUtf16String());

    // add artificial @article with same coords as in @srcItem
    bool AddArtificialAtSameNodes(const NGzt::TMessage& article, const TGztResultItem& srcItem, const TUtf16String& title = TUtf16String());


    // Returns true if subtree corresponding to @item is quoted (on any level).
    bool HasQuotes(const TGztResultItem& item) const;

    // True if ther is multitoken which has sub-nodes both contained in @item and not contained in @item.
    bool HasIncompleteMultitoken(const TGztResultItem& item) const;

    static void Sort(TVector<TGztResultPosition>& results);

    void ExtractItemData(size_t index, TGztResultData& dst) const;
    void ExtractData(TVector<TGztResultData>& result) const;


    typedef TSet<const NGzt::TDescriptor*> TExternalDescriptors;
    void SetExternalDescriptors(const TExternalDescriptors& descriptors) {
        ExternalDescriptors = &descriptors;
    }

    inline bool IsExternalResult(const TGztResultItem& item) const {
        // External gzt-results are output in rrr with other external rule results.
        // By default all gzt-results are considered external.
        return ExternalDescriptors == nullptr ||
               ExternalDescriptors->find(item.GetType()) != ExternalDescriptors->end();
    }


    TUtf16String DebugString() const;

private:
    class TImpl;
    THolder<TImpl> Impl;


    const TExternalDescriptors* ExternalDescriptors;


private:
    // mapper from index to TGztResultItem
    inline const TGztResultItem& operator()(size_t index) const {
        return operator[](index);
    }

    inline TIndexIterator IterateResults(const TVector<size_t>& indexes) const {
        NIter::TVectorIterator<const size_t> indexIterator(indexes);
        return TIndexIterator(indexIterator, this);
    }

    class TSieve {
    public:
        TSieve(const TImpl& impl, TVector<TGztResultPosition>& results);
        void Next(const TRichRequestNode& curNode);

    private:
        struct TCandidate {
            TGztResultPosition Item;
            size_t NodeIndex;
        };

        void AddCandidate(const TGztResultItem& item, size_t pos);

    private:
        const TImpl* Impl;
        TIntrusivePtr<NGzt::TSuperNodeVector> Snapshot;
        TList<TCandidate> Candidates;
        TVector<TGztResultPosition>& Results;
    };
};

// Contains same data as TGztResultItem in simple static form. It could be transferred easily
// to outer users of wizard.gazetteer (e.g. web-report) as part of wizard results.
struct TGztResultData
{
    ui32 Position, WordCount;
    TUtf16String FoundKey;      // not serialized, used in printwzrd
    TString ArticleType;
    TString ArticleTitle;  // utf8
    TString ArticleData;   // protobuf-serialized binary of article content

    // Textual representation of article content
    // Not serialized (used for imitating rule-results)
    TString ArticleText;


    // empty constructor is required for TVector<TGztResultData> serialization
    inline TGztResultData()
        : Position(0), WordCount(0)
    {
    }

    TGztResultData(const TGztResultItem& item, ui32 startIndex, ui32 stopIndex) {
        Reset(item, startIndex, stopIndex);
    }

    void Reset(const TGztResultItem& item, ui32 startIndex, ui32 stopIndex);

    void Save(IOutputStream* output) const;
    void Load(IInputStream* input);
};

namespace NGzt {

// Adapts standard rich-tree iterator (derived from TBaseRichNodeIterator) to NGzt-style iterator.
template <class TRichTreeIterator>
class TRichTreeIteratorAdaptor
{
public:
    inline TRichTreeIteratorAdaptor()
        : Iter()
    {
    }

    inline explicit TRichTreeIteratorAdaptor(TRichTreeIterator& iter)
        : Iter(iter)
    {
    }

    inline bool Ok() const {
        return !Iter.IsDone();
    }

    inline void operator++() {
        ++Iter;
    }

    inline TRichRequestNode* operator->() const {
        return Iter.operator->();
    }

    inline TRichRequestNode& operator*() const {
        return *(Iter.operator->());
    }

private:
    TRichTreeIterator Iter;
};

} // namespace NGzt

// Template methods implementation

template <class TGztNodeIterator>
bool TGztResults::FindByContentNodeGztIterator(TGztNodeIterator& nodes, TVector<TGztResultPosition>& results) const
{
    TSieve sieve(*Impl, results);
    for (; nodes.Ok(); ++nodes)
        sieve.Next(*nodes);
    return results.size() > 0;
}

template <class TRichTreeIterator>
bool TGztResults::FindByContentNodeIterator(TRichTreeIterator& nodes, TVector<TGztResultPosition>& results) const
{
    NGzt::TRichTreeIteratorAdaptor<TRichTreeIterator> temp(nodes);
    return FindByContentNodeGztIterator(temp, results);
}
