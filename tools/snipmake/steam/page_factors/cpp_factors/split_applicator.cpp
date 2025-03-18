#include "split_applicator.h"
#include "common_factors.h"
#include "split_factors.h"

#include <kernel/matrixnet/mn_sse.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/hash_set.h>
#include <util/generic/set.h>
#include <util/generic/ymath.h>

namespace NSegmentator {

class TSplittableNodeTraverser : public NDomTree::IAbstractTraverser<TFactorNode*> {
public:
    TSplittableNodeTraverser(TFactorTree::TFactorNodes& nodes)
        : Nodes(nodes)
        , Current(Nodes.begin())
    {}

    TFactorNode* Next() override {
        while (!(Current == Nodes.end() || Current->Splittable.Get(false))) {
            ++Current;
        }
        TFactorNode* res = (Current == Nodes.end() ? nullptr : &*Current);
        ++Current;
        return res;
    }

private:
    TFactorTree::TFactorNodes& Nodes;
    TFactorTree::TFactorNodes::iterator Current;
};


// TSplitApplicator
void TSplitApplicator::Apply(bool useModel) {
    InitSplitMarks(FactorTree.GetRoot());
    CalcFactors();
    if (useModel) {
        ApplyModel();
    }
}

void TSplitApplicator::ApplyModel() {
    TVector<const float*> factors;
    for (const TFactorNode& node : TSplitNodeIterator(FactorTree)) {
        factors.push_back(node.SplitFactors->data());
    }
    TVector<double> splitResults(factors.size());
    MnSplit.DoCalcRelevs(factors.data(), splitResults.data(), factors.size());

    SetSplitMarks(splitResults);
}

void TSplitApplicator::InitSplitMarks(TFactorNode* factorNode) {
    if (factorNode->Splittable.IsActive()) {
        return;
    }

    if (factorNode->Node->IsText()) {
        factorNode->Splittable.Set(false);
        return;
    }
    const NHtml::TTag& tag = NHtml::FindTag(factorNode->Node->Tag());
    if (IsInlineTag(tag)) {
        factorNode->Splittable.Set(false);
        return;
    }
    if (IsMediaTag(tag)) {
        factorNode->Splittable.Set(false);
        return;
    }

    for (TFactorNode& child : TFactorTree::ChildTraversal(factorNode)) {
        InitSplitMarks(&child);
    }
    for (TFactorNode& child : TFactorTree::ChildTraversal(factorNode)) {
        if (child.Splittable.Get()) {
            factorNode->Splittable.Set(true);
            return;
        }
    }

    size_t nonEmptyChildren = 0;
    for (TFactorNode& child : TFactorTree::ChildTraversal(factorNode)) {
        if (!child.IsEmpty()) {
            ++nonEmptyChildren;
            if (nonEmptyChildren > 1) {
                break;
            }
        }
    }
    factorNode->Splittable.Set(nonEmptyChildren > 1);
}

void TSplitApplicator::CalcFactors() {
    TSplittableNodeTraverser traverser(FactorTree.Nodes);
    TFactorNode* node = traverser.Next();
    TFactors nodeFactors;
    TFactors topNodeFactors;
    while (nullptr != node) {
        CalcNodeFactors(nodeFactors, node);
        CalcTopNodeFactors(topNodeFactors, nodeFactors, node);
        node->SplitFactors.Reset(new TFactors());
        CalcNodeExtFactors(*node->SplitFactors, topNodeFactors);
        node = traverser.Next();
    }
}

void TSplitApplicator::SetSplitMarks(const TVector<double>& splitResults) {
    TVector<double>::const_iterator splitResult = splitResults.begin();
    TFactorTree::TFactorNodes& nodes = FactorTree.Nodes;
    TFactorTree::TFactorNodes::iterator node = nodes.begin();
    TFactorNode* nextNotInSubtreeNode = &*node;
    while (true) {
        while (node != nodes.end() && nextNotInSubtreeNode != &*node) {
            if (node->Splittable.Get(false)) {
                ++splitResult;
            }
            node->Splittable.Set(false);
            ++node;
        }
        if (node == nodes.end()) {
            break;
        }

        TFactorTree::TFactorNodes::iterator nextNode = node;
        ++nextNode;
        nextNotInSubtreeNode = (nextNode == nodes.end() ? nullptr : &*nextNode);

        if (node->Splittable.Get(false)) {
            if (!NeedToSplit(*splitResult)) {
                node->Splittable.Set(false);
                nextNotInSubtreeNode = TFactorTree::CalcNodeFromRight(&*node);
            }
            ++splitResult;
        } else {
            node->Splittable.Set(false);
            nextNotInSubtreeNode = TFactorTree::CalcNodeFromRight(&*node);
        }

        node = nextNode;
    }
}

static const size_t CHAR_FACTORS_OFFSET = 50;
static const size_t TREE_FACTORS_OFFSET = 1213;
void TSplitApplicator::CalcNodeFactors(TFactors& factors, TFactorNode* factorNode) const {
    factors.clear();
    // TODO: other factors

    TFactors charFactors;
    CalcCharFactors(charFactors, factorNode, FactorTree);
    factors.resize(CHAR_FACTORS_OFFSET, 0);
    factors.insert(factors.end(), charFactors.begin(), charFactors.end());

    TFactors treeFactors;
    CalcTreeFactors(treeFactors, factorNode, FactorTree);
    factors.resize(TREE_FACTORS_OFFSET, 0);
    factors.insert(factors.end(), treeFactors.begin(), treeFactors.end());
}

void TSplitApplicator::CalcTopNodeFactors(TFactors& factors, const TFactors& nodeFactors, TFactorNode* factorNode) const {
    factors.clear();
    factors.resize(ESplitFactors::SF_COUNT, 0);

    TVector<TVector<float>> childrenNodeFactors(nodeFactors.size());
    TFactors childNodeFactors;
    for (TFactorNode& child : TFactorTree::ChildTraversal(factorNode)) {
        CalcNodeFactors(childNodeFactors, &child);
        for (size_t i = 0; i < childrenNodeFactors.size(); ++i) {
            if (!IsNan(childNodeFactors[i])) {
                childrenNodeFactors[i].push_back(childNodeFactors[i]);
            }
        }
    }

    /*
     * see https://beta.wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/Projects/Segmentator/factors/
     *
     * switch (formulaFactorId % 5):
     *     case 0: nodeFactor
     *     case 1: Amplitude(children) / nodeFactor
     *     case 2: Avg(children) / nodeFactor
     *     case 3: Middle(children) / nodeFactor
     *     case 4: Var(children)
     */

    factors[SF_FACTOR_8] = CalcVar(childrenNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_0]);
    factors[SF_FACTOR_9] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_2];
    factors[SF_FACTOR_10] = CalcVar(childrenNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_2]);
    factors[SF_FACTOR_11] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_6];
    factors[SF_FACTOR_12] = CalcVar(childrenNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_27]);
    factors[SF_FACTOR_13] = CalcVar(childrenNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_30]);
    factors[SF_FACTOR_14] = CalcVar(childrenNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_50]);
    factors[SF_FACTOR_15] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_60];
    factors[SF_FACTOR_16] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_61];
    factors[SF_FACTOR_17] = CalcVar(childrenNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_173]);

    factors[SF_FACTOR_61] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_0];
    factors[SF_FACTOR_62] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_1];
    factors[SF_FACTOR_63] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_2];
    factors[SF_FACTOR_64] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_3];
    factors[SF_FACTOR_65] = CalcVar(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_3]);
    factors[SF_FACTOR_66] = DivideOrNaN(CalcAvg(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_4]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_4]);
    factors[SF_FACTOR_67] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_5];
    factors[SF_FACTOR_68] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_10];
    factors[SF_FACTOR_69] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_11];
    factors[SF_FACTOR_70] = DivideOrNaN(CalcAvg(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_11]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_11]);
    factors[SF_FACTOR_71] = DivideOrNaN(CalcMiddle(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_11]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_11]);
    factors[SF_FACTOR_72] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_12];
    factors[SF_FACTOR_73] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_13];
    factors[SF_FACTOR_74] = DivideOrNaN(CalcMiddle(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_13]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_13]);
    factors[SF_FACTOR_75] = DivideOrNaN(CalcAmplitude(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_14]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_14]);
    factors[SF_FACTOR_76] = CalcVar(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_14]);
    factors[SF_FACTOR_77] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15];
    factors[SF_FACTOR_78] = DivideOrNaN(CalcAmplitude(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15]);
    factors[SF_FACTOR_79] = DivideOrNaN(CalcMiddle(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15]);
    factors[SF_FACTOR_80] = CalcVar(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15]);
    factors[SF_FACTOR_81] = DivideOrNaN(CalcMiddle(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_19]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_19]);
    factors[SF_FACTOR_82] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_56];
    factors[SF_FACTOR_83] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_91];
    factors[SF_FACTOR_84] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_93];
    factors[SF_FACTOR_85] = DivideOrNaN(CalcAvg(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98]);
    factors[SF_FACTOR_86] = DivideOrNaN(CalcMiddle(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98]);
    factors[SF_FACTOR_87] = DivideOrNaN(CalcMiddle(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_99]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_99]);
    factors[SF_FACTOR_88] = DivideOrNaN(CalcAvg(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_100]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_100]);
    factors[SF_FACTOR_89] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_103];
    factors[SF_FACTOR_90] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_133];
    factors[SF_FACTOR_91] = CalcVar(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_133]);
    factors[SF_FACTOR_92] = CalcVar(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_153]);
    factors[SF_FACTOR_93] = DivideOrNaN(CalcAvg(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_156]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_156]);
    factors[SF_FACTOR_94] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_255];
    factors[SF_FACTOR_95] = DivideOrNaN(CalcMiddle(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_255]),
                                        nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_255]);
    factors[SF_FACTOR_96] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_270];
    factors[SF_FACTOR_97] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_278];
    factors[SF_FACTOR_98] = CalcVar(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_278]);
    factors[SF_FACTOR_99] = CalcVar(childrenNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_323]);
}

void TSplitApplicator::CalcNodeExtFactors(TFactors& nodeExtFactors, const TFactors& factors) const {
    TSet<ui32> factorIds;
    MnSplit.UsedFactors(factorIds);
    CalcExtFactors(nodeExtFactors, factors, factorIds, MnSplit.GetNumFeats(), ESplitFactors::SF_COUNT);
}

bool TSplitApplicator::NeedToSplit(double splitResult) const {
    return splitResult > SplitResultBorder;
}


void SetUpToRootSplittable(TFactorNode* factorNode) {
    while (nullptr != factorNode) {
        factorNode->Splittable.Set(true);
        factorNode = factorNode->Parent;
    }
}


// TAwareSplitApplicator
void TAwareSplitApplicator::ApplyModel() {
    SetNodesFakeGuids();
    SetNodesUnsplittable();

    const NJson::TJsonValue::TArray& choicesArray = CorrectSegmentation["choice"].GetArraySafe();
    THashSet<TSplitGuid, TSplitGuidHash> splitGuids;
    for (const NJson::TJsonValue& guidJson : choicesArray) {
        const TString& guid = guidJson.GetStringSafe();
        TSplitGuid splitGuid(guid);
        splitGuids.insert(splitGuid);
    }

    for (TFactorNode& node : TSplitNodeIterator(FactorTree)) {
        TStringBuf guid = node.GetGuid();
        THashSet<TSplitGuid, TSplitGuidHash>::iterator nodeSplitGuid = \
            splitGuids.find(TSplitGuid(TString(guid.data(), guid.size())));
        if (nodeSplitGuid == splitGuids.end()) {
            continue;
        }
        if (nodeSplitGuid->IsNative) {
            SetUpToRootSplittable(node.Parent);
        } else {
            // node with current guid is splittable (guid_i is unsplittable)
            SetUpToRootSplittable(&node);
        }
    }
}

void TAwareSplitApplicator::SetNodesFakeGuids() {
    TSplittableNodeTraverser traverser(FactorTree.Nodes);
    TFactorNode* node = traverser.Next();
    while (nullptr != node) {
        ui32 curFakeId = 0;
        for (TFactorNode& child : TFactorTree::ChildTraversal(node)) {
            const NHtml::TTag& tag = NHtml::FindTag(child.Node->Tag());
            if (child.Node->IsText() ||
                !(!IsInlineTag(tag) || TFactorTree::HasNonEmptyBlockDeeper(&child)))
            {
                if (!child.IsEmpty()) {
                    child.MetaFactors.FakeGuidIdx = curFakeId;
                }
            } else {
                ++curFakeId;
                if (!child.Splittable.Get(false) && !child.IsEmpty()) {
                    child.MetaFactors.FakeGuidIdx = curFakeId++;
                }
            }
        }
        node = traverser.Next();
    }
}

void TAwareSplitApplicator::SetNodesUnsplittable() {
    for (TFactorNode& node : FactorTree.Nodes) {
        node.Splittable.Set(false);
    }
}

}  // NSegmentator
