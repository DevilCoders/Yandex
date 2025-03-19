#include "richnode.h"
#include <util/charset/unidata.h>
#include <util/generic/yexception.h>
#include "printrichnode.h"

#include "nodeiterator.h"

#include <kernel/qtree/richrequest/builder/richtreebuilderaux.h>

using namespace NSearchQuery;

static void FixMarkup(TRichRequestNode& node) {
    // move single word markups from leaf-children upper to parent
    for (size_t i = 0; i != node.Children.size(); ++i) {
        if (IsWord(*node.Children[i]) && !node.Children[i]->Markup().Empty()) {
            for (TAllMarkupIterator j(node.Children[i]->MutableMarkup()); !j.AtEnd(); ++j) {
                j->Range.Beg = i;
                j->Range.End = i;
                node.MutableMarkup().Add(*j);
            }
            node.Children[i]->MutableMarkup().Clear();
        }
    }
}

static bool IsSubTree(const TRichRequestNode& child, bool ignoreParens) {
    return IsAndOp(child) && !child.WordInfo && child.MiscOps.empty() && (ignoreParens || !child.IsEnclosed());
}

// (a (b c)) => (a b c)
void CollectSubTree(TRichNodePtr& tree, bool ignoreParens) {
    if (!IsAndOp(*tree))
        return;

    if (tree->Children.size() == 1 && tree->IsEnclosed())
        ignoreParens = true;

    for (size_t i = 0; i < tree->Children.size(); ++i) {
        CollectSubTree(tree->Children.MutableNode(i));
        if (IsSubTree(*tree->Children[i], ignoreParens)) {
            i += tree->Children.FlattenOneLevel(i);
        }
    }

    FixMarkup(*tree);
}

static bool IsWhole(const TRichRequestNode& node, const TMarkupItem& mi) {
    const size_t rangeLast = node.Children.empty() ? 0 : node.Children.size() - 1;
    return mi.Range.Beg == 0 && mi.Range.End == rangeLast;
}

TSynonymData* GetSynonymData(TMarkupItem& it) {
    if (it.Data->MarkupType() == TTechnicalSynonym::SType)
        return &(it.GetDataAs<TTechnicalSynonym>());
    if (it.Data->MarkupType() == TSynonym::SType)
        return &(it.GetDataAs<TSynonym>());
    return nullptr;
}

template<class TSynType>
TVector<TMarkupItem> MoveSynonymsImpl(const TRange& nodeCoord, const TSynType& parSyn, bool allowComb)  {
    TVector<TMarkupItem> addMarkup;

    FixMarkup(*parSyn.SubTree);

    for (TAllMarkupIterator i(parSyn.SubTree->MutableMarkup()); !i.AtEnd(); ++i) {
        if (IsWhole(*parSyn.SubTree, *i)) {
            i->Range = nodeCoord;
            addMarkup.push_back(*i);
            TSynonymData* newSyn = GetSynonymData(addMarkup.back());
            if (!newSyn)
                continue;
            newSyn->SetBestFormClass(Max(newSyn->GetBestFormClass(), parSyn.GetBestFormClass()));
            newSyn->RemoveType(TE_MARK);
            newSyn->AddType(TE_MARK2);
        } else if (allowComb) {
            const TSynonymData* oldData = GetSynonymData(*i);
            if (!oldData)
                continue;

            TMarkupDataPtr newMu = parSyn.Clone();

            TSynType* newSyn = CheckedCast<TSynType*>(newMu.Get());
            Y_ASSERT(IsAndOp(*newSyn->SubTree));
            newSyn->SubTree->ReplaceChildren(i->Range.Beg, i->Range.Size(), oldData->SubTree);
            newSyn->SubTree->MutableMarkup().Clear();
            newSyn->SetBestFormClass(Max(newSyn->GetBestFormClass(), oldData->GetBestFormClass()));
            if (IsAndOp(*oldData->SubTree))
                newSyn->SubTree->Children.FlattenOneLevel(i->Range.Beg);
            addMarkup.push_back(TMarkupItem(nodeCoord, newMu));
        }
    }
    parSyn.SubTree->MutableMarkup().Clear();

    return addMarkup;
}

template<class TSynType>
static void MoveSynonyms(TRichRequestNode& parent, const TRange& nodeCoord, TSynType& parSyn, bool allowComb)  {
    for (TForwardMarkupIterator<TSynonym, false> i(parSyn.SubTree->MutableMarkup()); !i.AtEnd(); ++i)
        MoveSynonyms(*parSyn.SubTree, i->Range, i.GetData(), false);
    for (TForwardMarkupIterator<TTechnicalSynonym, false> i(parSyn.SubTree->MutableMarkup()); !i.AtEnd(); ++i)
        MoveSynonyms(*parSyn.SubTree, i->Range, i.GetData(), false);

    TVector<TMarkupItem> addMarkup = MoveSynonymsImpl(nodeCoord, parSyn, allowComb);

    for (size_t i = 0; i < addMarkup.size(); ++i)
        parent.MutableMarkup().Add(addMarkup[i]);
}

namespace {
    struct TFlatSynonyms {
        void operator () (TRichRequestNode& node) const {
            for (TForwardMarkupIterator<TSynonym, false> i(node.MutableMarkup()); !i.AtEnd(); ++i) {
                MoveSynonyms(node, i->Range, i.GetData(), true);
            }
            for (TForwardMarkupIterator<TTechnicalSynonym, false> i(node.MutableMarkup()); !i.AtEnd(); ++i) {
                MoveSynonyms(node, i->Range, i.GetData(), true);
            }
        }
    };

    struct TShrinkWordInstances {
        bool Flatten;

        TShrinkWordInstances(bool flatten)
            : Flatten(flatten)
        {}

        void operator () (TRichRequestNode& node) const {
            if (!node.WordInfo || !node.WordInfo->IsLemmerWord())
                return;
            TWordInstanceUpdate(*node.WordInfo).RepairLLL();
            TWordInstanceUpdate(*node.WordInfo).ShrinkLemmas(NLanguageMasks::NoBastardsInSearch(), true);
            if (Flatten)
                TWordInstanceUpdate(*node.WordInfo).FlattenLemmas(~NLanguageMasks::LemmasInIndex());
        }
    };

    struct TUpdateRedundantLemmas {
        void operator () (TRichRequestNode& node) const {
            if (!node.WordInfo)
                return;
            TWordInstanceUpdate(*node.WordInfo).SpreadFixListLemmas();
            TWordInstanceUpdate(*node.WordInfo).UpdateRedundantLemmas();
        }
    };

    struct TConvertSynonyms {
        void operator () (TRichRequestNode& node) const {
            for (TForwardMarkupIterator<TTechnicalSynonym, false> i(node.MutableMarkup()); !i.AtEnd(); ++i)
                node.AddMarkup(i->Range.Beg, i->Range.End, new TSynonym(i.GetData()));
            node.MutableMarkup().GetItems(TTechnicalSynonym::SType).clear();
        }
    };

    struct TSortMarkup {
        bool Deep;

        TSortMarkup(bool deep)
            : Deep(deep)
        {}
        void operator () (TRichRequestNode& node) const {
            if (Deep)
                node.MutableMarkup().DeepSort();
            else
                node.MutableMarkup().Sort();
        }
    };

    struct TConvertMarks {
        struct TCrossRange {
            NSearchQuery::TRange Range;

            TCrossRange(NSearchQuery::TRange range)
            : Range(range)
            {
            }

            bool operator () (const TMarkupItem& item) const {
                if (!CheckType(item))
                    return false;
                return !!item.Range.Intersect(Range) && !item.Range.Contains(Range) && !Range.Contains(item.Range);
            }

            bool CheckType(const TMarkupItem& item) const {
                EMarkupType t = item.Data->MarkupType();
                return t != MT_SYNTAX;
            }
        };
    public:
        const bool IgnoreCrossMakrup;
        const EThesExtType SynType = TE_MARK;
        const EThesExtType NewSynType = TE_MARK2;

    public:
        TConvertMarks(bool ignoreCrossMakrup)
            : IgnoreCrossMakrup(ignoreCrossMakrup)
        {
        }

        // Consider synonyms of @c synType as marks for replacement
        TConvertMarks(bool ignoreCrossMarkup, EThesExtType synType, EThesExtType newSynType)
            : IgnoreCrossMakrup(ignoreCrossMarkup)
            , SynType(synType)
            , NewSynType(newSynType)
        {
        }

        static bool HasCrossMarkup(const TRichRequestNode& node, NSearchQuery::TRange range) {
            typedef NSearchQuery::TGeneralCheckMarkupIterator<NSearchQuery::TAllMarkupConstIterator, TCrossRange> TCrossRangeIterator;
            return !TCrossRangeIterator(node.Markup(), TCrossRange(range)).AtEnd();
        }

        TVector<TMarkupItem> GetReplaceableMarks(TRichRequestNode& node) const {
            TVector<TMarkupItem> res;

            typedef NSearchQuery::TForwardMarkupIterator<TSynonym, false> TSynonymIterator;
            NSearchQuery::TCheckMarkupIterator<TSynonymIterator, NSearchQuery::TSynonymTypeCheck> it (
                TSynonymIterator(node.MutableMarkup()),
                NSearchQuery::TSynonymTypeCheck(SynType)
                );

            TSet<NSearchQuery::TRange> ranges;
            for (; !it.AtEnd(); ++it) {
                if (IgnoreCrossMakrup || !HasCrossMarkup(node, it->Range) && !ranges.contains(it->Range)) {
                    res.push_back(*it);
                    ranges.insert(it->Range);
                }
            }
            return res;
        }

        static void SubSeq(const TNodeSequence& seq, NSearchQuery::TRange range, TNodeSequence& dist) {
            Y_ASSERT(range.Beg <= range.End);
            Y_ASSERT(range.End < seq.size());
            dist.Clear();
            for (size_t i = range.Beg; i <= range.End; ++i) {
                dist.Append(seq[i], seq.ProxBefore(i));
            }
            for (TAllMarkupConstIterator it(seq.Markup()); !it.AtEnd(); ++it) {
                if (range.Contains(it->Range) && range != it->Range) {
                    TMarkupItem itm = *it;
                    itm.Range.Shift(-(int)range.Beg);
                    dist.MutableMarkup().Add(itm);
                }
            }
        }

        static TRichNodePtr SubTree(const TNodeSequence& seq, NSearchQuery::TRange range) {
            if (range.Size() == 1)
                return seq[range.Beg];

            Y_ASSERT(range.Size() > 1);
            TRichNodePtr node = CreateEmptyRichNode();
            node->SetPhrase(PHRASE_PHRASE);
            node->OpInfo = DefaultPhraseOpInfo;
            SubSeq(seq, range, node->Children);
            return node;
        }

        static void RepairMarkup(TVector<TMarkupItem>& replaceableMarks, size_t beg, size_t end, size_t num) {
            int shift = int(end - beg) - num;
            if (!shift)
                return;
            for (size_t i = 0; i < replaceableMarks.size(); ++i) {
                if (replaceableMarks[i].Range.Empty())
                    continue;
                if (replaceableMarks[i].Range.Contains(beg) != replaceableMarks[i].Range.Contains(end - 1)) {
                    replaceableMarks[i].Range.Reset(); // make it empty
                    continue;
                }

                if (replaceableMarks[i].Range.Beg >= end)
                    replaceableMarks[i].Range.Beg -= shift;
                if (replaceableMarks[i].Range.End >= end)
                    replaceableMarks[i].Range.End -= shift;
            }
        }

        void ReplaceMarks(TRichRequestNode& node, TVector<TMarkupItem>& replaceableMarks) const {
            for (size_t i = 0; i < replaceableMarks.size(); ++i) {
                if (replaceableMarks[i].Range.Empty())
                    continue;
                TRichNodePtr newSynTree = SubTree(node.Children, replaceableMarks[i].Range);
                TSynonym& syn = replaceableMarks[i].GetDataAs<TSynonym>();
                if (syn.SubTree->Children.empty()) {
                    node.Children.EquivalentReplace(replaceableMarks[i].Range.Beg, replaceableMarks[i].Range.End + 1, syn.SubTree);
                    RepairMarkup(replaceableMarks, replaceableMarks[i].Range.Beg, replaceableMarks[i].Range.End + 1, 1);
                } else {
                    node.Children.EquivalentReplace(replaceableMarks[i].Range.Beg, replaceableMarks[i].Range.End + 1, syn.SubTree->Children);
                    RepairMarkup(replaceableMarks, replaceableMarks[i].Range.Beg, replaceableMarks[i].Range.End + 1, syn.SubTree->Children.size());
                }
                syn.SubTree = newSynTree;
                syn.RemoveType(SynType);
                syn.AddType(NewSynType);
            }
        }

        void operator () (TRichRequestNode& node) const {
            TVector<TMarkupItem> replaceableMarks = GetReplaceableMarks(node);
            ReplaceMarks(node, replaceableMarks);

            if (node.Children.size() == 1 && node.OpInfo.Op == oAnd) {
                TRichNodePtr theChild = node.Children[0];

                for (TAllMarkupIterator it(node.MutableMarkup()); !it.AtEnd(); ++it) {
                    theChild->MutableMarkup().Add(*it);
                }
                theChild->MiscOps.insert(theChild->MiscOps.end(), node.MiscOps.begin(), node.MiscOps.end());

                node.Swap(*theChild); // now theChild->Children[0] == theChild
                theChild->Children.Clear();
            }
        }
    };

    struct TUnconvertMarks {
        void operator () (TRichRequestNode& node) const {
            if (Y_LIKELY(node.Children)) {
                TConvertMarks(false, TE_MARK2, TE_MARK)(node);
                return;
            }

            using TSynonymIterator = NSearchQuery::TForwardMarkupIterator<TSynonym, false>;
            NSearchQuery::TCheckMarkupIterator<TSynonymIterator, NSearchQuery::TSynonymTypeCheck> it(
                TSynonymIterator(node.MutableMarkup()),
                NSearchQuery::TSynonymTypeCheck(TE_MARK2)
            );

            if (!it.AtEnd()) {
                TSynonym& syn = it->GetDataAs<TSynonym>();
                // Markup pointer should not be swapped, it should point from node.Chilren to syn.Subtree, even after swap.
                TMarkup markup;
                markup.Swap(node.Children.MutableMarkup());
                node.Swap(*syn.SubTree);
                markup.Swap(node.Children.MutableMarkup());
                syn.RemoveType(TE_MARK2);
                syn.AddType(TE_MARK);
            }
        }
    };
}

template<class T>
static void UpdateAllTrees(TRichNodePtr& node, const T& upd) {
    for (size_t i = 0; i < node->Children.size(); ++i)
        UpdateAllTrees(node->Children.MutableNode(i), upd);
    for (size_t i = 0; i < node->MiscOps.size(); ++i)
        UpdateAllTrees(node->MiscOps[i], upd);
    for (TForwardMarkupIterator<TSynonym, false> i(node->MutableMarkup()); !i.AtEnd(); ++i)
        UpdateAllTrees(i.GetData().SubTree, upd);
    for (TForwardMarkupIterator<TTechnicalSynonym, false> i(node->MutableMarkup()); !i.AtEnd(); ++i)
        UpdateAllTrees(i.GetData().SubTree, upd);

    upd(*node);
}

// a ^ (b ^ c) => a ^ b ^ c
static void FlatSynonyms(TRichNodePtr& node)  {
    for (size_t i = 0; i < node->Children.size(); ++i)
        FlatSynonyms(node->Children.MutableNode(i));

    TFlatSynonyms fl;
    fl(*node);
}

static void ShrinkWordInstances(TRichNodePtr& root, bool flatten) {
    UpdateAllTrees(root, TShrinkWordInstances(flatten));
}

void UpdateRedundantLemmas(TRichNodePtr& root) {
    UpdateAllTrees(root, TUpdateRedundantLemmas());
}

// TTechnicalSynonym => TSynonym
void ConvertSynonyms(TRichNodePtr& root) {
    UpdateAllTrees(root, TConvertSynonyms());
}

// (mp 3) ^ mp3 => mp3 ^ (mp 3)
static void ConvertMarks(TRichNodePtr& root)  {
    UpdateAllTrees(root, TConvertMarks(false));
}

// mp3 ^ (mp 3) => (mp 3) ^ mp3
// Undoes the results of ConvertMarks
static void UnconvertMarks(TRichNodePtr& root)  {
    UpdateAllTrees(root, TUnconvertMarks());
}

static void SortMarkup(TRichNodePtr& root, bool deep)  {
    UpdateAllTrees(root, TSortMarkup(deep));
}

void UpdateRichTree(TRichNodePtr& tree, bool sortMarkup, bool splitMultitokens) {
    ShrinkWordInstances(tree, false);
    UpdateRedundantLemmas(tree);

    FlatSynonyms(tree);
    CollectSubTree(tree);
    ConvertSynonyms(tree);
    if (!splitMultitokens)
        ConvertMarks(tree);
    FlatSynonyms(tree);

    SortMarkup(tree, sortMarkup);
}

void UnUpdateRichTree(TRichNodePtr& tree) {
    UnconvertMarks(tree);
}

bool DeleteMarkerNode(TRichNodePtr& parent, const TRichRequestNode* deleteTrg)  {
    if ((parent->IsOrdinaryWord() || parent->IsStopWord() || parent->IsExactWord()) && parent == deleteTrg) {
        return true;
    }
    if (parent->Children.size() == 1)
        return false;
    for (size_t i = 0 ; i < parent->Children.size(); ++i) {
        TRichNodePtr chNode = parent->Children[i];
        if (DeleteMarkerNode(chNode, deleteTrg)) { // we must delete chNode
            parent->RemoveChildren(i, i+1);
            break;
        }
        if (parent->Children[i]->IsBinaryOp() && parent->Children[i]->Children.size() == 1) {
            parent->ReplaceChild(i, chNode);
            break;
        }
    }
    if (parent->IsBinaryOp() && parent->Children.size() == 1) {
        TVector<TRichNodePtr> miscOps = parent->MiscOps;
        TMarkup markup = parent->Markup();
        parent = parent->Children[0];
        parent->MutableMarkup().Add(TAllMarkupIterator(markup));
        parent->MiscOps.insert(parent->MiscOps.end(), miscOps.begin(), miscOps.end());
    }
    return false;
}

// ignores markup
void SimpleUnMarkRichTree(TRichNodePtr& node) {
    UpdateAllTrees(node, TConvertMarks(true));
}

bool FixPlainNumbers(TRichNodePtr& node)
{
    bool fixed = false;

    for (TAllWordIterator it(node); !it.IsDone(); ++it) {
        if (!it->WordInfo->IsInteger()) {
            continue;
        }

        TAutoPtr<TWordNode> wi = TWordNode::CreatePlainIntegerNode(
            it->WordInfo->GetNormalizedForm(), it->WordInfo->GetFormType());

        EStickySide stick = STICK_NONE;
        const bool stWord = it->WordInfo->IsStopWord(stick);
        wi->SetStopWord(stWord, stick);
        it->WordInfo.Reset(wi.Release());
        fixed = true;
    }

    return fixed;
}
