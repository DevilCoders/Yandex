#include "gztres.h"

#include "richnodeiter.h"
#include <kernel/gazetteer/common/coverage.h>

#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/printrichnode.h>

#include <util/generic/noncopyable.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>
#include <util/ysaveload.h>

using namespace NGzt;

typedef TRichRequestNode::TNodesVector TNodePtrVector;
typedef TArticleIter<TNodePtrVector> TDefaultArticleIterator;

TGztResultItem::TGztResultItem(const TSuperNodeVector& nodes, const TArticlePtr& art, size_t begin, size_t end)
    : TArticlePtr(art)
    , Phrase(nodes)
    , StartIndex(begin)
    , StopIndex(end)
{
    Y_ASSERT(StartIndex < StopIndex && StopIndex <= Phrase.Size());
}

template <typename TInput>
void TGztResultItem::ResetWords(const TArticleIter<TInput>& iter) {
    const TPhraseWords<TInput>& words = iter.GetWords();
    size_t size = words.Size();

    Words.clear();
    Words.reserve(size);
    for (size_t i = 0; i < size; ++i)
        Words.push_back(TWord(TUtf16String(words.GetString(i)), words.IsLemma(i)));
}

void TGztResultItem::ResetWords(const NGzt::TSuperNodeVector& nodes, size_t begin, size_t end) {
    Words.clear();
    Words.reserve(end - begin);
    for (size_t i = begin; i < end; ++i)
        Words.push_back(TWord(GetRichRequestNodeText(nodes[i]->Node.Get()), false));
}

bool TGztResultItem::TWord::WasFoundOn(const TRichRequestNode& node) const
{
    const TWordNode* w = node.WordInfo.Get();

    if (w != nullptr && IsLemma && w->IsLemmerWord()) {
        TRichRequestNodeIterator::TLemmaIter lemma(w->LemsBegin(), w->LemsEnd());
        for (; lemma.Ok(); ++lemma)
            if (lemma->GetLemma() == Text)
                return true;

        return false;
    } else
        return IsNodeText(&node, Text);
}

bool TGztResultItem::CompareNodes(size_t index, const TRichRequestNode& checked) const
{
    Y_ASSERT(index < Size());
    return Phrase[StartIndex + index]->Node->GetId() == checked.GetId()
        && (IsArtificial() || Words[index].WasFoundOn(checked));
}

bool TGztResultItem::TrySkip(size_t& index, const TRichRequestNode& target) const {
    if (index >= Size())
        return false;

    if (CompareNodes(index, target)) {
        ++index;
        return true;
    }

    if (target.Children.empty())
        return false;

    size_t tmp = index;
    for (size_t i = 0; i < target.Children.size(); ++i)
        if (!TrySkip(tmp, *target.Children[i]))
            return false;

    index = tmp;
    return true;
}

bool TGztResultItem::FindLongestSubPhrase(const TRichRequestNode& node, size_t& curIndex) const {
    // TODO: rewrite using TrySkip()
    Y_ASSERT(curIndex < Size());
    if (CompareNodes(curIndex, node)) {
        ++curIndex;
        return true;
    } else if (node.Children.empty())
        // holes allowed only before StartIndex node is found
        return curIndex == 0;

    for (size_t i = 0; curIndex < Size() && i < node.Children.size(); ++i)
        if (!FindLongestSubPhrase(*node.Children[i], curIndex))
            return false;

    return true;
}

bool TGztResultItem::IsValidForTree(const TRichRequestNode& treeRoot) const
{
    if (Y_UNLIKELY(StartIndex >= StopIndex))
        return true;    // empty (?) but valid

    size_t curIndex = 0;
    return FindLongestSubPhrase(treeRoot, curIndex) && curIndex >= Size();
}

TUtf16String TGztResultItem::RestoreFoundKey(TUtf16String* buffer) const
{
    TUtf16String res;
    if (buffer == nullptr)
        buffer = &res;
    else
        buffer->clear();

    for (size_t i = 0; i < Words.size(); ++i) {
        if (i > 0)
            buffer->append(' ');
        if (!Words[i].IsLemma)
            buffer->append('!');
        buffer->append(Words[i].Text);
    }
    return *buffer;
}

TUtf16String TGztResultItem::DebugString() const {
    TUtf16String res;
    res.AppendAscii("[").AppendAscii(::ToString(GetId())).AppendAscii("]\t");
    res.append(GetTypeName()).AppendAscii(" ").append(GetTitle()).AppendAscii("\t:: ").append(RestoreFoundKey());
    return res;
}


// Helper class: contains a collection of lower-level word nodes (words or/and multi-tokens) from user phrase
struct TNodeCollection : TNonCopyable {
    TSuperNodeIndex Index;

    TSuperNodeVector SmallTokens, BigTokens;

    // these tables are empty, if no multitokens
    TVector<size_t> SmallToBig;    // small-token index -> index of including ref-token

    // Saved states of a request tree at the moments when a rule makes
    // an association of some artificial gzt-result with particular tree segment.
    // We'll want to refer snapshot pharses by pointer, so a vector cannot be used
    // as it can move its elements in memory on re-allocations. But a list is OK here.
    TList<TSuperNodeVector> Snapshots;

    inline TNodeCollection() {
    }

    TNodeCollection(const TRichNodePtr& userPhrase) {
        Reset(userPhrase);
    }

    void Clear();
    void Reset(const TRichNodePtr& userPhrase);

    // Returns true if at least one multi-token was found
    bool CollectBig(const TSuperNode& root);
    void CollectSmall(const TSuperNode& root, TSuperNodeVector& smallTokens) const;

    const TSuperNodeVector* FindPhrase(const TSuperNodeVector& phrase, size_t& shift) const;
    const TSuperNodeVector& MakeSnapshot(const TRichRequestNode& root);
    const TSuperNodeVector& MakeSnapshot(const TSuperNodeVector& phrase, size_t& shift);

    // Conversion of multitoken/word index into ref-token index
    inline size_t GetRefIndex(size_t index, const TVector<size_t>& indexToRef) const;

    inline void GetRefCoords(const TGztResultItem& item, size_t& startIndex, size_t& stopIndex) const {
/*
        // ref-coords are position relative to BigTokens (if any)
        const TSuperNodeVector& refTok = BigTokens.Size() ? BigTokens : SmallTokens;
        if (!refTok.LocateAnySpan(*item.Phrase[item.StartIndex], *item.Phrase[item.StopIndex - 1], startIndex, stopIndex))
            ythrow yexception() << "Cannot locate gzt-result " << item.DebugString() << Endl;
*/
        if (&item.Phrase != &BigTokens) {
            startIndex = GetRefIndex(item.StartIndex, SmallToBig);
            stopIndex = GetRefIndex(item.StopIndex, SmallToBig);
        } else {
            startIndex = item.StartIndex;
            stopIndex = item.StopIndex;
        }
    }

    void DebugString(IOutputStream& out) const;
};

void TNodeCollection::Clear()
{
    SmallTokens.Clear();
    BigTokens.Clear();
    SmallToBig.clear();
    Snapshots.clear();

    Index.Clear();
}

void TNodeCollection::Reset(const TRichNodePtr& userPhrase)
{
    Clear();
    const TSuperNode& root = Index.GetOrInsert(userPhrase);
    if (CollectBig(root)) {
        SmallToBig.reserve(BigTokens.Size()*2);
        for (size_t i = 0; i < BigTokens.Size(); ++i) {
            size_t j = SmallTokens.Size();
            CollectSmall(*BigTokens[i], SmallTokens);
            Y_ASSERT(SmallTokens.Size() > j);      // should contain at least one small token
            for (; j < SmallTokens.Size(); ++j)
                SmallToBig.push_back(i);
        }
    } else
        SmallTokens.Swap(BigTokens);

    // finally BigTokens will be non-empty only if there is at least one multi-token in @userPhrase
    // otherwise it will be empty.
    // @SmallTokens will always contain all simple words (non multi-token).

    //DebugString(Cerr);
}


bool TNodeCollection::CollectBig(const TSuperNode& root) {
    if (root.IsSmall()) {
        BigTokens.PushBack(root);
        return false;
    } else if (root.IsBig()) {
        BigTokens.PushBack(root);
        return true;
    } else {
        bool hasBig = false;
        for (size_t i = 0; i < root.Children.size(); ++i)
            if (CollectBig(*root.Children[i]))
                hasBig = true;
        return hasBig;
    }
}

void TNodeCollection::CollectSmall(const TSuperNode& node, TSuperNodeVector& collection) const {
    if (node.IsSmall())
        collection.PushBack(node);
    else {
        for (size_t i = 0; i < node.Children.size(); ++i)
            CollectSmall(*node.Children[i], collection);
    }
}

const TSuperNodeVector* TNodeCollection::FindPhrase(const TSuperNodeVector& phrase, size_t& shift) const {
    if (SmallTokens.LocateHere(phrase, shift))
        return &SmallTokens;
    if (BigTokens.LocateHere(phrase, shift))
        return &BigTokens;
    for (const TSuperNodeVector& i : Snapshots)
        if (i.LocateHere(phrase, shift))
            return &i;

    return nullptr;
}

const TSuperNodeVector& TNodeCollection::MakeSnapshot(const TRichRequestNode& root) {
    const TSuperNode& s = Index.GetOrInsert(root);

    TSuperNodeVector snapshot;
    CollectSmall(s, snapshot);

    size_t shift;
    const TSuperNodeVector* existing = FindPhrase(snapshot, shift);
    if (existing)
        return *existing;

    Snapshots.push_back(TSuperNodeVector());
    Snapshots.back().Swap(snapshot);
    return Snapshots.back();
}

const TSuperNodeVector& TNodeCollection::MakeSnapshot(const TSuperNodeVector& phrase, size_t& shift) {
    // fast try: compare addresses
    shift = 0;
    if (&phrase == &SmallTokens)
        return SmallTokens;

    if (&phrase == &BigTokens)
        return BigTokens;

    for (const TSuperNodeVector& i : Snapshots) {
        if (&phrase == &i)
            return i;
    }

    // fallback: search by content
    const TSuperNodeVector* existing = FindPhrase(phrase, shift);
    if (existing)
        return *existing;

    Snapshots.push_back(TSuperNodeVector());
    TSuperNodeVector& snapshot = Snapshots.back();
    for (size_t i = 0; i < phrase.Size(); ++i)
        snapshot.PushBack(Index.GetOrInsert(phrase[i]->Node));
    return snapshot;
}

size_t TNodeCollection::GetRefIndex(size_t index, const TVector<size_t>& indexToRef) const {
    if (BigTokens.Size() == 0)
        return index;
    return index < indexToRef.size() ? indexToRef[index] : indexToRef.size();
}
/*
static inline void GztInputDebugString(const TNodePtrVector& words, IOutputStream& out) {
    for (TRichTreePtrIterator wordIt(words); wordIt.Ok(); ++wordIt) {
        out << wordIt.GetWordString() << " :: ";
        for (TRichTreePtrIterator::TLemmaIter lemmaIt = wordIt.IterLemmas(); lemmaIt.Ok(); ++lemmaIt)
            out << wordIt.GetLemmaString(lemmaIt) << ", ";
        out << "\n";
    }
}
*/


void TNodeCollection::DebugString(IOutputStream& out) const {
    //TNodePtrVector Words, MultiTokens
    out << "SMALL:\n";
    for (size_t i = 0; i < SmallTokens.Size(); ++i)
        out << SmallTokens[i]->DebugString() << Endl;

    if (BigTokens.Size() > 0) {
        out << "BIG:\n";
        for (size_t i = 0; i < BigTokens.Size(); ++i)
            out << BigTokens[i]->DebugString() << Endl;
    }

    out << "=========================" << Endl;
}






class TGztResults::TImpl {
    friend class TGztResults;
public:
    inline TImpl(const TGazetteer* gazetteer, size_t itemsLimit = 0)
        : Gazetteer(gazetteer)
        , ItemPool(sizeof(TGztResultItem)*256)
        , ArtificialSize(0)
        , ItemsLimit(itemsLimit)
    {
    }

    inline TImpl(TRichNodePtr inputTree, const TGazetteer* gazetteer, size_t itemsLimit = 0)
        : Gazetteer(gazetteer)
        , ItemPool(sizeof(TGztResultItem)*256)
        , ArtificialSize(0)
        , ItemsLimit(itemsLimit)
    {
        Reset(inputTree, false);
    }

    inline ~TImpl() {
        Clear();
    }

    void Reset(TRichNodePtr inputTree, bool extraForms, bool normalize = false);
    void Clear();

    void CollectParentNodes(TRichNodePtr& tree, TRichRequestNode* parent);

    // returns old size of `Items`
    template <typename TIter, typename TForms>
    size_t CollectResults(const TGazetteer& gzt, TIter& it, const TForms& small, const TForms& big);
    size_t CollectResults(const TGazetteer& gzt, bool extraForms = false, bool normalize = false);

    void AddFromGzt(const TGazetteer& gzt, size_t itemsLimit);

    void BuildIndexes(size_t startIndex);

    void AddSingleItem(TGztResultItem* item);

    bool AddArtificial(const TMessage& article, const TRichRequestNode& first, const TRichRequestNode& last, const TRichRequestNode& parent, const TUtf16String& title = TUtf16String());
    bool AddArtificial(const TMessage& article, const TGztResultItem& srcPosItem, const TUtf16String& title = TUtf16String());

    inline ui32 NextArtificialId();

    // mapper from index iterator to TGztResultItem
    inline const TGztResultItem& operator()(const size_t index) const {
        return *Items[index];
    }

    // Finds lowest node in the tree which has all @res's nodes among its children (i.e. fully contains @res)
    // @firstChild and @lastChild (if not NULL) will contain a minimal span of children holding this @res.
    const TSuperNode* FindCommonAncestor(const TGztResultItem& res, size_t* firstChild = nullptr, size_t* lastChild = nullptr) const;

    static inline bool Less(const TGztResultPosition& a, const TGztResultPosition& b) {
        return *a < *b;
    }


    void ExtractItemData(const TGztResultItem& item, TGztResultData& dst) const;

    inline void ExtractData(TVector<TGztResultData>& results) const {
        for (size_t i = 0; i < Items.size(); ++i) {
            results.emplace_back();
            ExtractItemData(*Items[i], results.back());
        }
    }

private:
    TGztResultItem* NewRawItem(const TSuperNodeVector& nodes, const TArticlePtr& art, size_t begin, size_t end) {
        TGztResultItem* ptr = ItemPool.Allocate<TGztResultItem>();
        return new (ptr) TGztResultItem(nodes, art, begin, end);
    }

    template <typename TInput>
    TGztResultItem* NewItem(const TGazetteer& gzt, const TArticleIter<TInput>& iter, const TSuperNodeVector& nodes) {
        TArticlePtr art(*iter, gzt.ArticlePool());

        // if this is my gazetteer, add as is
        // otherwise it cannot be added directly, because it's id is not from this->Gazetteer.
        // we should copy its content into artifical article (with artificial id)
        if (&gzt != Gazetteer)
            art = TArticlePtr::MakeExternalArticle(NextArtificialId(), *art, art.GetTitle());

        size_t begin = iter.GetWords().FirstWordIndex();
        size_t end = begin + iter.GetWords().Size();

        TGztResultItem* ptr = NewRawItem(nodes, art, begin, end);
        ptr->ResetWords(iter);
        return ptr;
    }

    // pure artificial result (no gzt-iterator)
    TGztResultItem* NewArtificialItem(const TMessage& object, const TSuperNodeVector& nodes, size_t begin, size_t end, const TUtf16String& title = TUtf16String()) {
        TArticlePtr art = TArticlePtr::MakeExternalArticle(NextArtificialId(), object, title);
        TGztResultItem* ptr = NewRawItem(nodes, art, begin, end);
        ptr->ResetWords(nodes, begin, end);
        return ptr;
    }

    void ClearItems() {
        for (size_t i = 0; i < Items.size(); ++i)
            Items[i]->~TGztResultItem();
        Items.clear();
    }

private: // data //

    const TGazetteer* Gazetteer;
    TDefaultArticleIterator ArticleIter;

    // Source lower-level nodes (with two level of tokenization possibly) groupped by enclosing user-phrase
    TNodeCollection Nodes;

    TMemoryPool ItemPool;

    // All gzt-results found in source nodes, points to items allocated on ItemPool.
    TVector<TGztResultItem*> Items;

    // Number of artificially created articles
    size_t ArtificialSize;

    // Indexes: contains index from Items for corresponding gzt-results.
    typedef THashMap<const TDescriptor*, TVector<size_t> > TDescriptorMap;
    TDescriptorMap ExactTypeIndex;
    TDescriptorMap BaseTypeIndex;

    THashMap<TUtf16String, size_t> TitleIndex;

    typedef THashMap<long, TVector<size_t> > TNodeMap;     // node id -> list of all gzt-results indexes associated with this node
    //TNodeMap ParentNodeIndex;
    TNodeMap StartNodeIndex;

    /* For now it works only in CollectResults */
    size_t ItemsLimit = 0;
};

void TGztResults::TImpl::Clear()
{
    Nodes.Clear();
    ClearItems();

    ArtificialSize = 0;

    ExactTypeIndex.clear();
    BaseTypeIndex.clear();
    TitleIndex.clear();

    StartNodeIndex.clear();
}

void TGztResults::TImpl::Reset(TRichNodePtr inputTree, bool extraForms, bool normalize)
{
    Clear();
    Nodes.Reset(inputTree);

    if (Gazetteer != nullptr) {
        BuildIndexes(CollectResults(*Gazetteer, extraForms, normalize));
    }
}

template <typename TInput>
static inline bool HasBigTokens(const TSuperNodeVector& nodes, const TArticleIter<TInput>& it) {
    size_t firstNode = it.GetWords().FirstWordIndex();
    size_t lastNode = it.GetWords().LastWordIndex();
    for (size_t j = firstNode; j <= lastNode; ++j)
        if (nodes[j]->IsBig())
            return true;
    return false;
}

template <typename TIter, typename TForms>
size_t TGztResults::TImpl::CollectResults(const TGazetteer& gzt, TIter& it, const TForms& small, const TForms& big) {
    size_t initSize = Items.size();
    if ((ItemsLimit) && (initSize >= ItemsLimit)) {
        return initSize;
    }

    // first search gzt-articles in simple (non-multi) words
    gzt.IterArticles(small, &it);
    for (; it.Ok(); ++it) {
        Items.push_back(NewItem(gzt, it, Nodes.SmallTokens));
        if ((ItemsLimit) && (Items.size() >= ItemsLimit)) {
            return initSize;
        }
    }
    // then if there are any multi-words, search gzt-articles again
    // but this time add found results to Items only if they cover at least one multi-word
    if (Nodes.BigTokens.Size() > 0) {
        gzt.IterArticles(big, &it);
        for (; it.Ok(); ++it)
            if (HasBigTokens(Nodes.BigTokens, it)) {
                Items.push_back(NewItem(gzt, it, Nodes.BigTokens));
                if ((ItemsLimit) && (Items.size() >= ItemsLimit)) {
                    return initSize;
                }
            }
    }
    return initSize;
}

size_t TGztResults::TImpl::CollectResults(const TGazetteer& gzt, bool extraForms, bool normalize) {
    const auto& small = Nodes.SmallTokens.RichNodes();
    const auto& big = Nodes.BigTokens.RichNodes();
    if (extraForms) {
        Y_ASSERT(!normalize);
        TArticleIter<TExtraFormsRichNodes> it;
        return CollectResults(gzt, it, TExtraFormsRichNodes(small), TExtraFormsRichNodes(big));
    }
    if (normalize) {
        TArticleIter<TNormalizedRichNodes> it;
        return CollectResults(gzt, it, TNormalizedRichNodes(small), TNormalizedRichNodes(big));
    }
    return CollectResults(gzt, ArticleIter, small, big);
}

void TGztResults::TImpl::BuildIndexes(size_t startIndex)
{
    Y_ASSERT(Gazetteer != nullptr);
    const TProtoPool& descriptors = Gazetteer->ProtoPool();
    for (size_t i = startIndex; i < Items.size(); ++i) {
        const TGztResultItem& item = *Items[i];
        TitleIndex[item.GetTitle()] = i;

        const TDescriptor* type = item.GetType();
        ExactTypeIndex[type].push_back(i);
        BaseTypeIndex[type].push_back(i);

        // map all base types of @type to @item (in BaseTypeIndex)
        ui32 curIndex = 0;
        if (descriptors.GetIndex(type, curIndex)) {
            ui32 baseIndex = descriptors.GetBaseIndex(curIndex);
            while (baseIndex != curIndex) {
                BaseTypeIndex[descriptors[baseIndex]].push_back(i);
                curIndex = baseIndex;
                baseIndex = descriptors.GetBaseIndex(curIndex);
            }
        }

        StartNodeIndex[item.FirstNode().GetId()].push_back(i);
    }
}

void TGztResults::TImpl::AddSingleItem(TGztResultItem* item)
{
    size_t index = Items.size();
    Items.push_back(item);

    // refresh some indexes
    const TDescriptor* type = item->GetType();
    ExactTypeIndex[type].push_back(index);
    BaseTypeIndex[type].push_back(index);
    StartNodeIndex[item->FirstNode().GetId()].push_back(index);
}

bool TGztResults::TImpl::AddArtificial(const TMessage& article, const TRichRequestNode& first, const TRichRequestNode& last, const TRichRequestNode& parent, const TUtf16String& title)
{
    const TSuperNodeVector& snapshot = Nodes.MakeSnapshot(parent);
    const TSuperNode& sfirst = Nodes.Index.GetOrInsert(first);
    const TSuperNode& slast = Nodes.Index.GetOrInsert(last);

    size_t begin = 0, end = 0;
    if (!snapshot.LocateSmallSpan(sfirst, slast, begin, end))
        return false;     // TODO: throw exception?

    AddSingleItem(NewArtificialItem(article, snapshot, begin, end, title));
    return true;
}

bool TGztResults::TImpl::AddArtificial(const TMessage& article, const TGztResultItem& srcPosItem, const TUtf16String& title)
{
    size_t shift = 0;
    const TSuperNodeVector& snapshot = Nodes.MakeSnapshot(srcPosItem.Phrase, shift);
    TGztResultItem* newItem = NewArtificialItem(article, snapshot, srcPosItem.GetOriginalStartIndex() + shift,
                                                                   srcPosItem.GetOriginalStopIndex()  + shift, title);
    AddSingleItem(newItem);
    return true;
}

void TGztResults::TImpl::AddFromGzt(const TGazetteer &gzt, size_t itemsLimit) {
    ItemsLimit = itemsLimit ? Items.size() + itemsLimit : 0;
    BuildIndexes(CollectResults(gzt, /*extraForms=*/false));
}

inline ui32 TGztResults::TImpl::NextArtificialId() {
    ui32 res = Max<ui32>() - ArtificialSize - 1;    // do not use Max<ui32>() itself
    Y_ASSERT(!TArticlePool::IsValidOffset(res));
    ++ArtificialSize;
    return res;
}

const TSuperNode* TGztResults::TImpl::FindCommonAncestor(const TGztResultItem& res, size_t* firstChild, size_t* lastChild) const
{
    typedef THashSet<const TSuperNode*> TNodeSet;
    TNodeSet parents, children;
    for (TGztResultItem::TNodeIterator it = res.IterNodes(); it.Ok(); ++it) {
        const TSuperNode* sn = Nodes.Index.Get(it->Get());
        if (!sn)
            return nullptr;
        parents.insert(sn);
    }

    do {
        children.swap(parents);
        parents.clear();
        for (TNodeSet::const_iterator it = children.begin(); it != children.end(); ++it) {
            const TSuperNode* par = (*it)->Parent;
            if (par != nullptr)
                parents.insert(par);
        }
    } while (parents.size() > 1);

    if (parents.empty())
        return nullptr;
    else {
        const TSuperNode* result = *parents.begin();
        if (firstChild != nullptr && lastChild != nullptr) {
            for (*firstChild = 0; *firstChild < result->Children.size(); ++(*firstChild))
                if (children.contains(result->Children[*firstChild]))
                    break;
            for (*lastChild = result->Children.size(); *lastChild > *firstChild;) {
                --(*lastChild);
                if (*lastChild == *firstChild || children.contains(result->Children[*lastChild]))
                    break;
            }
        }
        return result;
    }
}

void TGztResults::TImpl::ExtractItemData(const TGztResultItem& item, TGztResultData& dst) const
{
    // Return position in terms of ref-tokens
    // Note that a partial multitoken item will be rounded to whole multitokens edges.
    // This is until we finally make the tree flat.
    size_t startIndex = 0, stopIndex = 0;
    Nodes.GetRefCoords(item, startIndex, stopIndex);
    // Round to whole ref-token boundary (avoiding empty gzt-items)
    if (stopIndex <= startIndex)
        stopIndex = startIndex + 1;
    dst.Reset(item, startIndex, stopIndex);
}

TGztResults::TGztResults(const TGazetteer* gazetteer, size_t itemsLimit)
    : Impl(new TImpl(gazetteer, itemsLimit))
    , ExternalDescriptors(nullptr)
{
}

TGztResults::TGztResults(TRichNodePtr inputTree, const TGazetteer* gazetteer, size_t itemsLimit)
    : Impl(new TImpl(inputTree, gazetteer, itemsLimit))
    , ExternalDescriptors(nullptr)
{
}

TGztResults::~TGztResults() {
}

void TGztResults::Reset(TRichNodePtr inputTree, bool normalize) {
    Impl->Reset(inputTree, false, normalize);
}

void TGztResults::ResetWithExtraForms(TRichNodePtr inputTree) {
    return Impl->Reset(inputTree, true);
}

void TGztResults::Clear() {
    Impl->Clear();
}

size_t TGztResults::Size() const {
    return Impl->Items.size();
}

const TGztResultItem& TGztResults::operator[](size_t index) const {
    return *Impl->Items[index];
}

const TGazetteer* TGztResults::Gazetteer() const {
    return Impl->Gazetteer;
}

TGztResults::TRangedForGztResultsIterator TGztResults::begin() const {
    return TRangedForGztResultsIterator(this, 0);
}

TGztResults::TRangedForGztResultsIterator TGztResults::end() const {
    return TRangedForGztResultsIterator(this, Impl->Items.size());
}

bool TGztResults::AddArtificial(const TMessage& article, const TRichRequestNode& first, const TRichRequestNode& last, const TRichRequestNode& parent, const TUtf16String& title) {
    return Impl->AddArtificial(article, first, last, parent, title);
}

bool TGztResults::AddArtificialAtSameNodes(const TMessage& article, const TGztResultItem& srcItem, const TUtf16String& title) {
    return Impl->AddArtificial(article, srcItem, title);
}

void TGztResults::AddFromGzt(const TGazetteer& gazetteer, size_t itemsLimit) {
    Impl->AddFromGzt(gazetteer, itemsLimit);
}

bool TGztResults::HasQuotes(const TGztResultItem& item) const {
    for (TGztResultItem::TNodeIterator it = item.IterNodes(); it.Ok(); ++it) {
        for (const TSuperNode* node = Impl->Nodes.Index.Get(it->Get()); node != nullptr; node = node->Parent)
            if (IsQuote(*node->Node))
                return true;
    }
    return false;
}

bool TGztResults::HasIncompleteMultitoken(const TGztResultItem& item) const {
    // if the first node in the middle of multitoken (or in the end)
    const TRichRequestNode* node = item.GetOriginalPhrase()[item.GetOriginalStartIndex()].Get();
    const TSuperNode* snode = Impl->Nodes.Index.Get(node);
    if (snode && !snode->StartsBig())
        return true;

    // if the last node in the middle of multitoken (or in the beginning)
    if (item.Size() > 1) {
        node = item.GetOriginalPhrase()[item.GetOriginalStopIndex() - 1].Get();
        snode = Impl->Nodes.Index.Get(node);
        if (snode && !snode->EndsBig())
            return true;
    }

    return false;
}

TGztResults::TIndexIterator TGztResults::FindByExactType(const TDescriptor* type) const
{
    TImpl::TDescriptorMap::const_iterator it = Impl->ExactTypeIndex.find(type);
    return (it != Impl->ExactTypeIndex.end()) ? IterateResults(it->second) : TIndexIterator();
}

TGztResults::TIndexIterator TGztResults::FindByBaseType(const TDescriptor* type) const
{
    TImpl::TDescriptorMap::const_iterator it = Impl->BaseTypeIndex.find(type);
    return (it != Impl->BaseTypeIndex.end()) ? IterateResults(it->second) : TIndexIterator();
}

const TGztResultItem* TGztResults::FindByTitle(const TUtf16String& title) const
{
    THashMap<TUtf16String, size_t>::const_iterator it = Impl->TitleIndex.find(title);
    return (it != Impl->TitleIndex.end()) ? Impl->Items[it->second] : nullptr;
}

void TGztResults::Sort(TVector<TGztResultPosition>& results) {
    return ::Sort(results.begin(), results.end(), TImpl::Less);
}


TGztResults::TSieve::TSieve(const TImpl& impl, TVector<TGztResultPosition>& results)
    : Impl(&impl)
    , Snapshot(new TSuperNodeVector())
    , Results(results)
{
}

void TGztResults::TSieve::AddCandidate(const TGztResultItem& item, size_t pos) {
    if (item.Size() > 0) {
        TCandidate c = {TGztResultPosition(&item, pos), 0};
        Candidates.push_back(c);
    }
}

void TGztResults::TSieve::Next(const TRichRequestNode& curNode) {
    size_t curPos = Snapshot->Size();
    const TSuperNode* snode = Impl->Nodes.Index.Get(&curNode);
    if (!snode) {
        Candidates.clear();
        return;
    }
    Snapshot->PushBack(*snode);

    // add to @candidates all gzt-results starting from current node
    auto iter = Impl->StartNodeIndex.find(curNode.GetId());
    if (iter != Impl->StartNodeIndex.end()) {
        const TVector<size_t>& indexes = iter->second;
        for (size_t j = 0; j < indexes.size(); ++j)
            AddCandidate(*Impl->Items[indexes[j]], curPos);
    }

    // now check all candidates to have their current node same as current node from input @nodes.
    // also check if node still has corresponding lemma
    for (TList<TCandidate>::iterator a = Candidates.begin(); a != Candidates.end();) {
        const TGztResultItem& item = *(a->Item);
        Y_ASSERT(a->NodeIndex < item.Size());

        if (!item.TrySkip(a->NodeIndex, curNode))
            a = Candidates.erase(a);
        else if (a->NodeIndex < item.Size())
            ++a;                        // not finished yet
        else {
            Results.push_back(a->Item); // finished, move it into results
            a = Candidates.erase(a);
        }
    }
}

struct TNodeReferencer
{
    inline const TRichRequestNode& operator()(const TRichRequestNode* node) const {
        return *node;
    }
};

struct TNodeConstReferencer
{
    inline const TRichRequestNode& operator()(const TRichNodePtr& node) const {
        return *node.Get();
    }
};

bool TGztResults::FindByContentNodes(const TConstNodesVector& nodes, TVector<TGztResultPosition>& results) const
{
    typedef const TRichRequestNode* TConstNodePtr;
    typedef NIter::TVectorIterator<const TConstNodePtr> TConstNodesVectorIterator;
    typedef NIter::TMappedIterator<const TRichRequestNode, TConstNodesVectorIterator, TNodeReferencer> TGztNodeIterator;

    TNodeReferencer mapper;
    TConstNodesVectorIterator iter1(nodes);
    TGztNodeIterator iter2(iter1, &mapper);
    return FindByContentNodeGztIterator<TGztNodeIterator>(iter2, results);
}

bool TGztResults::FindByContentNodes(const TVector<TRichRequestNode*>& nodes, TVector<TGztResultPosition>& results) const
{
    typedef TRichRequestNode* TNodePtr;
    typedef NIter::TVectorIterator<const TNodePtr> TConstNodesVectorIterator;
    typedef NIter::TMappedIterator<const TRichRequestNode, TConstNodesVectorIterator, TNodeReferencer> TGztNodeIterator;

    TNodeReferencer mapper;
    TConstNodesVectorIterator iter1(nodes);
    TGztNodeIterator iter2(iter1, &mapper);
    return FindByContentNodeGztIterator<TGztNodeIterator>(iter2, results);
}

bool TGztResults::FindByContentNodes(const TRichRequestNode::TNodesVector& nodes, TVector<TGztResultPosition>& results) const
{
    typedef NIter::TVectorIterator<const TRichNodePtr> TConstNodesVectorIterator;
    typedef NIter::TMappedIterator<const TRichRequestNode, TConstNodesVectorIterator, TNodeConstReferencer> TGztNodeIterator;

    TNodeConstReferencer mapper;
    TConstNodesVectorIterator iter1(nodes);
    TGztNodeIterator iter2(iter1, &mapper);
    return FindByContentNodeGztIterator<TGztNodeIterator>(iter2, results);
}

struct TGztResultPositionCoverageOp {

    static size_t Size(const TGztResultPosition& gztItem) {
        return gztItem.Size();
    }

    static size_t Start(const TGztResultPosition& gztItem) {
        return gztItem.GetStartIndex();
    }

    static size_t Stop(const TGztResultPosition& gztItem) {
        return gztItem.GetStopIndex();
    }
};

void TGztResults::SelectBestCoverage(TVector<TGztResultPosition>& results) const {
    TCoverage<TGztResultPosition, TGztResultPositionCoverageOp>::SelectBestCoverage(results);
}




void TGztResults::ExtractData(TVector<TGztResultData>& result) const {
    Impl->ExtractData(result);
}

void TGztResults::ExtractItemData(size_t index, TGztResultData& dst) const {
    Impl->ExtractItemData(*(Impl->Items[index]), dst);
}

TUtf16String TGztResults::DebugString() const {
    TUtf16String res;
    for (size_t i = 0; i < Impl->Items.size(); ++i)
         res.append(Impl->Items[i]->DebugString()).AppendAscii("\n");
    return res;
}

void TGztResultData::Reset(const TGztResultItem& item, ui32 startIndex, ui32 stopIndex)
{
    Position = startIndex;
    WordCount = stopIndex - startIndex;

    item.RestoreFoundKey(&FoundKey);
    ArticleType = item.GetType()->full_name();
    WideToUTF8(item.GetTitle(), ArticleTitle);

    TBlob blob = item.GetBinary();
    ArticleData.assign(blob.AsCharPtr(), blob.Size());

    TJsonPrinter().ToString(*item.GetArticle(), ArticleText);
}

void TGztResultData::Save(IOutputStream* output) const
{
    ::Save(output, Position);
    ::Save(output, WordCount);
    ::Save(output, ArticleType);
    ::Save(output, ArticleTitle);
    ::Save(output, ArticleData);
}

void TGztResultData::Load(IInputStream* input)
{
    ::Load(input, Position);
    ::Load(input, WordCount);
    ::Load(input, ArticleType);
    ::Load(input, ArticleTitle);
    ::Load(input, ArticleData);
}
