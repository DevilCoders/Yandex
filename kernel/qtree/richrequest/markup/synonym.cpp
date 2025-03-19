#include "synonym.h"
#include <kernel/qtree/richrequest/richnode.h>
#include <util/generic/ymath.h>

namespace NSearchQuery {
    TSynonymData::TSynonymData(TRichNodePtr subTree, TThesaurusExtensionId type, double relev, EFormClass match)
        : Type(type)
        , Relev(relev)
        , BestFormClass(match)
        , SubTree(subTree)
    {}

    TSynonymData::TSynonymData(const TSynonymData& synonym)
        : Type(synonym.Type)
        , Relev(synonym.Relev)
        , BestFormClass(synonym.BestFormClass)
        , SubTree(synonym.SubTree->Copy())
    {
    }


    TSynonymData::TSynonymData(const TUtf16String& text, const TCreateTreeOptions& options,
                               TThesaurusExtensionId type, double relev, EFormClass match)
        : Type(type)
        , Relev(relev)
        , BestFormClass(match)
    {
        SubTree = CreateRichNode(text, options);
    }

    bool TSynonymData::operator ==(const TSynonymData& s) const {
        return s.Type == Type && FuzzyEquals(s.Relev, Relev, 1e-6) && s.BestFormClass == BestFormClass && s.SubTree->Compare(SubTree.Get());
    }

    static bool MergeableWordNodes(const TWordNode* node1, const TWordNode* node2) {
        if (!node1 || !node2)
            return node1 == node2;
        if (node1->GetNormalizedForm() != node2->GetNormalizedForm() || node1->GetLangMask() != node1->GetLangMask())
            return false;
        const TWordInstance::TLemmasVector& lemmas1 = node1->GetLemmas();
        const TWordInstance::TLemmasVector& lemmas2 = node2->GetLemmas();
        if (lemmas1.size() != lemmas2.size())
            return false;
        for (size_t i = 0; i != lemmas1.size(); ++i)
            if (!SoftCompare(lemmas1[i], lemmas2[i]))
                return false;
        return true;
    }

    static bool HasMergeableWordNodes(const TRichRequestNode& tree1, const TRichRequestNode& tree2) {
        if (!tree1.WordInfo.Get() || !tree2.WordInfo.Get())
            return tree1.GetText() == tree2.GetText();
        return MergeableWordNodes(tree1.WordInfo.Get(), tree2.WordInfo.Get());
    }

    static bool MergeableProximity(const TProximity& prox1, const TProximity& prox2) {
        if (prox1.Level == prox2.Level) {
            // if there is no intersection
            if (prox1.Beg > prox2.End + 1 || prox1.End < prox2.Beg - 1)
                return false;
        }
        return true;
    }

    static bool MergeableRichTrees(const TRichRequestNode& tree1, const TRichRequestNode& tree2) {
        if (IsAttributeOrZone(tree1) || IsAttributeOrZone(tree2))
            return false;
        if (tree1.Children.size() != tree2.Children.size())
            return false;
        if (!HasMergeableWordNodes(tree1, tree2))
            return false;
        for (size_t i = 0; i != tree1.Children.size(); ++i) {
            if (!MergeableProximity(tree1.Children.ProxBefore(i), tree2.Children.ProxBefore(i)))
                return false;
            if (!MergeableRichTrees(*tree1.Children[i], *tree2.Children[i]))
                return false;
        }
        return true;
    }

    static bool SameWordNodes(const TWordNode* node1, const TWordNode* node2) {
        if (!node1 || !node2)
            return node1 == node2;
        return (MergeableWordNodes(node1, node2) && node1->GetFormType() == node2->GetFormType());
    }

    static bool HasSameWordNodes(const TRichRequestNode& tree1, const TRichRequestNode& tree2) {
        if (!tree1.WordInfo.Get() || !tree2.WordInfo.Get())
            return tree1.GetText() == tree2.GetText();
        return SameWordNodes(tree1.WordInfo.Get(), tree2.WordInfo.Get());
    }

    static bool SameRichTrees(const TRichRequestNode& tree1, const TRichRequestNode& tree2) {
        if (IsAttributeOrZone(tree1) || IsAttributeOrZone(tree2))
            return false;
        if (tree1.Children.size() != tree2.Children.size())
            return false;
        if (!HasSameWordNodes(tree1, tree2))
            return false;
        for (size_t i = 0; i != tree1.Children.size(); ++i) {
            if (!(tree1.Children.ProxBefore(i) == tree2.Children.ProxBefore(i)))
                return false;
            if (!SameRichTrees(*tree1.Children[i], *tree2.Children[i]))
                return false;
        }
        return true;
    }

    bool TSynonymData::HasSameRichTree(const TRichRequestNode& tree) const {
        return SameRichTrees(*SubTree, tree);
    }

    static void MergeWordNodes(TWordNode* destNode, const TWordNode* srcNode) {
        if (!destNode || !srcNode)
            return;
        if (destNode->GetFormType() == fExactLemma || destNode->GetFormType() == srcNode->GetFormType())
            return;
        if (srcNode->GetFormType() == fExactLemma || srcNode->GetFormType() == fExactWord)
               destNode->SetFormType(srcNode->GetFormType());
    }

    static void MergeProximity(TProximity& destProx, const TProximity& srcProx) {
        if (destProx.Level == srcProx.Level) {
                destProx.SetDistance(Min(destProx.Beg, srcProx.Beg), Max(destProx.End, srcProx.End), destProx.Level, destProx.DistanceType);
            } else if (srcProx.Level > destProx.Level) {
                destProx = srcProx;
            }
    }

    static void MergeRichTrees(TRichRequestNode& destTree, const TRichRequestNode& srcTree) {
        MergeWordNodes(destTree.WordInfo.Get(), srcTree.WordInfo.Get());
        for (size_t i = 0; i != destTree.Children.size(); ++i) {
            MergeRichTrees(*destTree.Children[i], *srcTree.Children[i]);
            MergeProximity(destTree.Children.ProxBefore(i), srcTree.Children.ProxBefore(i));
         }
    }


    bool TSynonymData::MergeData(TSynonymData& syn) {
        if (MergeableRichTrees(*SubTree, *syn.SubTree)) {
            AddType(syn.GetType());
            SetBestFormClass(Min(GetBestFormClass(), syn.GetBestFormClass()));
            MergeRichTrees(*SubTree, *syn.SubTree);
            return true;
        }
       return false; // not processed
    }
} // NSearchQuery
