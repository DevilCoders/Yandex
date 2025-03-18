#include "common_factors.h"

#include "block_stats.h"
#include "factor_tree.h"

#include <library/cpp/domtree/treetext.h>

#include <util/generic/algorithm.h>
#include <util/generic/iterator.h>
#include <util/generic/ymath.h>
#include <util/system/defaults.h>


namespace NSegmentator {

static const float EPS = 1e-6;


float DivideOrNaN(float a, float b){
    if (Abs(a) < EPS) {
        return 0;
    }
    return (Abs(b) < EPS) ? nanf("") : a / b;
}


template <typename T, typename TAcc>
float CalcAvg(const TVector<T>& elems, TAcc init) {
    if (elems.empty()) {
        return nanf("");
    }
    init = std::accumulate(elems.begin(), elems.end(), init);
    float elemsCount = elems.size();
    return init / elemsCount;
}

template <>
float CalcAvg(const TVector<ui32>& elems) {
    return CalcAvg(elems, ui64(0));
}

template <typename T>
float CalcAvg(const TVector<T>& elems) {
    return CalcAvg(elems, T(0));
}


template <typename T>
float CalcMiddle(const TVector<T>& elems) {
    if (elems.empty()) {
        return nanf("");
    }
    TVector<T> elemsCopy = elems;
    size_t middle = elemsCopy.size() / 2;
    NthElement(elemsCopy.begin(), elemsCopy.begin() + middle, elemsCopy.end());
    return elemsCopy[middle];
}


template <typename T>
float CalcMax(const TVector<T>& elems) {
    if (elems.empty()) {
        return nanf("");
    }
    return *MaxElement(elems.begin(), elems.end());
}


template <typename T>
float CalcAmplitude(const TVector<T>& elems) {
    if (elems.empty()) {
        return nanf("");
    }
    return *MaxElement(elems.begin(), elems.end()) - *MinElement(elems.begin(), elems.end());
}


template <typename T>
float CalcVar(const TVector<T>& elems) {
    if (elems.empty()) {
        return nanf("");
    }
    float avg = CalcAvg(elems);
    float numerator = 0;
    for (T el : elems) {
        numerator += (el - avg) * (el - avg);
    }
    float elemsCount = elems.size();
    return numerator / (elemsCount == 1 ? 1 : elemsCount - 1);
}


template float CalcAvg<float>(const TVector<float>& elems);
template float CalcMiddle<float>(const TVector<float>& elems);
template float CalcAmplitude<float>(const TVector<float>& elems);
template float CalcVar<float>(const TVector<float>& elems);


static void CalcRootDists(TFactorTree::TFactorNodes& nodes) {
    Y_ASSERT(!nodes.empty());
    for (TFactorNode& node : nodes) {
        if (nullptr == node.Parent) {
            node.MetaFactors.RootDist = 0;
        } else {
            node.MetaFactors.RootDist = node.Parent->MetaFactors.RootDist + 1;
        }
    }
}


static TVector<ui32> CalcLeavesDepths(TFactorNode* factorNode) {
    Y_ASSERT(nullptr != factorNode);
    TVector<ui32> leavesDepths;
    if (nullptr == factorNode->FirstChild) {
        leavesDepths.push_back(0);
    } else {
        for (TFactorNode& child : TFactorTree::ChildTraversal(factorNode)) {
            for (ui32 d : CalcLeavesDepths(&child)) {
                leavesDepths.push_back(d + 1);
            }
        }
    }
    factorNode->MetaFactors.AvgLeavesDepth = CalcAvg(leavesDepths);
    factorNode->MetaFactors.MedLeavesDepth = CalcMiddle(leavesDepths);
    factorNode->MetaFactors.MaxLeavesDepth = CalcMax(leavesDepths);
    return leavesDepths;
}


static TVector<ui32> CalcLeavesBlockDepths(TFactorNode* factorNode) {
    Y_ASSERT(nullptr != factorNode);
    TVector<ui32> leavesBlockDepths;
    if (nullptr == factorNode->FirstChild) {
        leavesBlockDepths.push_back(0);
    } else {
        for (TFactorNode& child : TFactorTree::ChildTraversal(factorNode)) {
            for (ui32 d : CalcLeavesBlockDepths(&child)) {
                if (child.Node->IsElement()) {
                    const NHtml::TTag& tag = NHtml::FindTag(child.Node->Tag());
                    if (!IsInlineTag(tag)) {
                        ++d;
                    }
                }
                leavesBlockDepths.push_back(d);
            }
        }
    }
    factorNode->MetaFactors.AvgLeavesBlockDepth = CalcAvg(leavesBlockDepths);
    factorNode->MetaFactors.MedLeavesBlockDepth = CalcMiddle(leavesBlockDepths);
    factorNode->MetaFactors.MaxLeavesBlockDepth = CalcMax(leavesBlockDepths);
    return leavesBlockDepths;
}


static void CalcBlocksStats(TFactorNode* factorNode) {
    Y_ASSERT(nullptr != factorNode);

    for (TFactorNode& child : TFactorTree::ChildTraversal(factorNode)) {
        CalcBlocksStats(&child);
    }
    factorNode->MetaFactors.BlockStats.Reset(new TBlockStats(factorNode->Node->NodeText()->RawTextNormal()));
}


void CalcMetaFactors(TFactorTree& factorTree) {
    CalcRootDists(factorTree.Nodes);
    CalcLeavesDepths(factorTree.GetRoot());
    CalcLeavesBlockDepths(factorTree.GetRoot());
    CalcBlocksStats(factorTree.GetRoot());
}


static ui32 CalcParentTagDist(TFactorNode* factorNode, const HT_TAG& tag) {
    ui32 dist = 0;
    while (nullptr != factorNode) {
        if (factorNode->Node->IsElement()) {
            const NHtml::TTag& curTag = NHtml::FindTag(factorNode->Node->Tag());
            if (curTag == tag) {
                return dist;
            }
        }
        ++dist;
        factorNode = factorNode->Parent;
    }
    return BAD_ID;
}


class TSiblingIterator : public TInputRangeAdaptor<TSiblingIterator> {
public:
    TSiblingIterator(TFactorNode* factorNode)
        : FactorNode(factorNode)
        , CurSibling((factorNode && factorNode->Parent) ? CalcNextNonEmpty(factorNode->Parent->FirstChild) : factorNode)
    {
        if (CurSibling == factorNode) {
            InitNodeIndex = 0;
        }
    }

    TFactorNode* Next() {
        TFactorNode* retval = CurSibling;
        if (nullptr != CurSibling) {
            ++CurNodeIndex;
            CurSibling = CalcNextNonEmpty(CurSibling->Next);
        }
        return retval;
    }

    ui32 GetInitNodeIndex() const {
        return InitNodeIndex;
    }

private:
    TFactorNode* CalcNextNonEmpty(TFactorNode* node) {
        while (nullptr != node) {
            if (node != FactorNode) {
                if (!node->IsEmpty()) {
                    break;
                }
            } else {
                InitNodeIndex = CurNodeIndex;
            }
            node = node->Next;
        }
        return node;
    }

private:
    TFactorNode* FactorNode;
    ui32 CurNodeIndex = 0;
    ui32 InitNodeIndex = BAD_ID;
    TFactorNode* CurSibling;
};


class TNonEmptyChildIterator : public TInputRangeAdaptor<TNonEmptyChildIterator> {
public:
    TNonEmptyChildIterator(TFactorNode* factorNode)
        : ChildIterator(factorNode)
    {
    }

    TFactorNode* Next() {
        TFactorNode* retval = ChildIterator.Next();
        while (nullptr != retval && retval->IsEmpty()) {
            retval = ChildIterator.Next();
        }
        return retval;
    }

private:
    TChildIterator ChildIterator;
};


static ui32 CalcSiblingsCountTag(TFactorNode* factorNode, const HT_TAG& tag) {
    ui32 siblingsCountTag = 0;
    for (const TFactorNode& node : TSiblingIterator(factorNode)) {
        if (node.Node->IsElement()) {
            const NHtml::TTag& curTag = NHtml::FindTag(node.Node->Tag());
            if (curTag == tag) {
                ++siblingsCountTag;
            }
        }
    }
    return siblingsCountTag;
}


static ui32 CalcGrandChildrenCountTag(TFactorNode* factorNode, const HT_TAG& tag) {
    ui32 grandChildCountTag = 0;
    for (TFactorNode& child : TNonEmptyChildIterator(factorNode)) {
        for (const TFactorNode& grandChild : TNonEmptyChildIterator(&child)) {
            const NHtml::TTag& curTag = NHtml::FindTag(grandChild.Node->Tag());
            if (curTag == tag) {
                ++grandChildCountTag;
            }
        }
    }
    return grandChildCountTag;
}


static ui32 CalcSubtreeCountTag(TFactorNode* factorNode, const TFactorTree& factorTree,
                                  const HT_TAG& tag, bool useSubtreeRoot = true) {
    ui32 subtreeCountTag = 0;
    for (const TFactorNode& node : TSubtreeIterator(factorNode, factorTree, useSubtreeRoot)) {
        const NHtml::TTag& curTag = NHtml::FindTag(node.Node->Tag());
        if (curTag == tag) {
            ++subtreeCountTag;
        }
    }
    return subtreeCountTag;
}


// Factors description: https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/Projects/Segmentator/factors/
void CalcCharFactors(TFactors& factors, TFactorNode* factorNode, TFactorTree& factorTree) {
    Y_ASSERT(factors.empty());

    // TODO: other factors
    factors.resize(ECommonCharFactors::CCF_COUNT, 0);

    TBlockStats& doc = *factorTree.GetRoot()->MetaFactors.BlockStats;
    TBlockStats& block = *factorNode->MetaFactors.BlockStats;

    factors[CCF_FACTOR_0] = block.BlockStats.Text.size();  // 50
    factors[CCF_FACTOR_2] = block.BlockStats.AlphasCount;  // 52
    factors[CCF_FACTOR_5] = DivideOrNaN(block.BlockStats.UpperAlphasCount, doc.BlockStats.UpperAlphasCount);  // 55
    factors[CCF_FACTOR_6] = block.BlockStats.DigitsCount;  // 56
    factors[CCF_FACTOR_7] = DivideOrNaN(block.BlockStats.DigitsCount, doc.BlockStats.DigitsCount);  // 57
    factors[CCF_FACTOR_9] = DivideOrNaN(block.BlockStats.PunctsCount, doc.BlockStats.PunctsCount);  // 59
    factors[CCF_FACTOR_24] = DivideOrNaN(block.BlockStats.UpperAlphasCount, block.BlockStats.AlphasCount);  // 74
    factors[CCF_FACTOR_26] = DivideOrNaN(block.BlockStats.AlphasCount, block.BlockStats.Text.size());  // 76
    factors[CCF_FACTOR_27] = DivideOrNaN(block.BlockStats.AlphasCount * doc.BlockStats.Text.size(),
                                         block.BlockStats.Text.size() * doc.BlockStats.AlphasCount);  // 77
    factors[CCF_FACTOR_28] = DivideOrNaN(block.BlockStats.DigitsCount, block.BlockStats.AlphasCount);  // 78
    factors[CCF_FACTOR_29] = DivideOrNaN(block.BlockStats.DigitsCount * doc.BlockStats.AlphasCount,
                                         block.BlockStats.AlphasCount * doc.BlockStats.DigitsCount);  // 79
    factors[CCF_FACTOR_30] = DivideOrNaN(block.BlockStats.PunctsCount, block.BlockStats.AlphasCount);  // 80
    factors[CCF_FACTOR_48] = DivideOrNaN(block.BlockStats.EndPunctsCount, block.BlockStats.UpperAlphasCount);  // 98
    factors[CCF_FACTOR_50] = DivideOrNaN(block.BlockStats.MidPunctsCount, block.BlockStats.AlphasCount);  // 100
    factors[CCF_FACTOR_52] = DivideOrNaN(block.BlockStats.MidPunctsCount, block.BlockStats.UpperAlphasCount);  // 102
    factors[CCF_FACTOR_57] = DivideOrNaN(block.BlockStats.AlphasCount * doc.BlockStats.WordsCount,
                                         block.BlockStats.WordsCount * doc.BlockStats.AlphasCount);  // 107
    factors[CCF_FACTOR_60] = DivideOrNaN(block.BlockStats.AlphasCount, block.ParsStats.size());  // 110
    factors[CCF_FACTOR_61] = DivideOrNaN(block.BlockStats.AlphasCount * doc.ParsStats.size(),
                                         block.ParsStats.size() * doc.BlockStats.AlphasCount);  // 111
    factors[CCF_FACTOR_62] = DivideOrNaN(block.BlockStats.UpperAlphasCount, block.BlockStats.WordsCount);  // 112
    factors[CCF_FACTOR_65] = DivideOrNaN(block.BlockStats.UpperAlphasCount * doc.SentsStats.size(),
                                         block.SentsStats.size() * doc.BlockStats.UpperAlphasCount);  // 115
    factors[CCF_FACTOR_68] = DivideOrNaN(block.BlockStats.DigitsCount, block.BlockStats.WordsCount);  // 118
    factors[CCF_FACTOR_69] = DivideOrNaN(block.BlockStats.DigitsCount * doc.BlockStats.WordsCount,
                                         block.BlockStats.WordsCount * doc.BlockStats.DigitsCount);  // 119
    factors[CCF_FACTOR_71] = DivideOrNaN(block.BlockStats.DigitsCount * doc.SentsStats.size(),
                                         block.SentsStats.size() * doc.BlockStats.DigitsCount);  // 121
    factors[CCF_FACTOR_76] = DivideOrNaN(block.BlockStats.PunctsCount, block.SentsStats.size());  // 126
    factors[CCF_FACTOR_78] = DivideOrNaN(block.BlockStats.PunctsCount, block.ParsStats.size());  // 128
    factors[CCF_FACTOR_79] = DivideOrNaN(block.BlockStats.PunctsCount * doc.ParsStats.size(),
                                         block.ParsStats.size() * doc.BlockStats.PunctsCount);  // 129

    TVector<size_t> blockParsUpperAlphasCounts;
    TVector<size_t> blockParsAlphasCounts;
    TVector<size_t> blockParsEndPunctsCounts;
    blockParsUpperAlphasCounts.reserve(block.ParsStats.size());
    blockParsAlphasCounts.reserve(block.ParsStats.size());
    blockParsEndPunctsCounts.reserve(block.ParsStats.size());
    for (const TTextStats& blockParStats : block.ParsStats) {
        blockParsUpperAlphasCounts.push_back(blockParStats.UpperAlphasCount);
        blockParsAlphasCounts.push_back(blockParStats.AlphasCount);
        blockParsEndPunctsCounts.push_back(blockParStats.EndPunctsCount);
    }

    TVector<size_t> docParsEndPunctsCounts;
    docParsEndPunctsCounts.reserve(doc.ParsStats.size());
    for (const TTextStats& docParStats : doc.ParsStats) {
        docParsEndPunctsCounts.push_back(docParStats.EndPunctsCount);
    }

    TVector<size_t> blockSentsUpperAlphasCounts;
    TVector<size_t> blockSentsEndPunctsCounts;
    TVector<size_t> blockWordsInSentsCounts;
    blockSentsUpperAlphasCounts.reserve(block.SentsStats.size());
    blockSentsEndPunctsCounts.reserve(block.SentsStats.size());
    blockWordsInSentsCounts.reserve(block.SentsStats.size());
    for (const TTextStats& blockSentStats : block.SentsStats) {
        blockSentsUpperAlphasCounts.push_back(blockSentStats.UpperAlphasCount);
        blockSentsEndPunctsCounts.push_back(blockSentStats.EndPunctsCount);
        blockWordsInSentsCounts.push_back(blockSentStats.WordsCount);
    }

    TVector<size_t> docSentsEndPunctsCounts;
    TVector<size_t> docWordsInSentsCounts;
    docSentsEndPunctsCounts.reserve(doc.SentsStats.size());
    docWordsInSentsCounts.reserve(doc.SentsStats.size());
    for (const TTextStats& docSentStats : doc.SentsStats) {
        docSentsEndPunctsCounts.push_back(docSentStats.EndPunctsCount);
        docWordsInSentsCounts.push_back(docSentStats.WordsCount);
    }

    factors[CCF_FACTOR_136] = CalcMiddle(blockParsUpperAlphasCounts);  // 186
    factors[CCF_FACTOR_138] = CalcMiddle(blockParsAlphasCounts);  // 188
    factors[CCF_FACTOR_154] = DivideOrNaN(CalcMiddle(blockSentsUpperAlphasCounts), CalcMiddle(blockWordsInSentsCounts));  // 204
    factors[CCF_FACTOR_170] = DivideOrNaN(CalcMiddle(blockSentsEndPunctsCounts), CalcMiddle(blockWordsInSentsCounts));  // 220
    factors[CCF_FACTOR_172] = DivideOrNaN(CalcMiddle(blockSentsEndPunctsCounts) * CalcMiddle(docWordsInSentsCounts),
                                          CalcMiddle(blockWordsInSentsCounts) * CalcMiddle(docSentsEndPunctsCounts));  // 222
    factors[CCF_FACTOR_173] = DivideOrNaN(CalcMiddle(blockParsEndPunctsCounts) * CalcMiddle(doc.SentsInParsCounts),
                                          CalcMiddle(block.SentsInParsCounts) * CalcMiddle(docParsEndPunctsCounts));  // 223
}


void CalcTreeFactors(TFactors& factors, TFactorNode* factorNode, TFactorTree& factorTree) {
    Y_ASSERT(factors.empty());

    // TODO: other factors
    factors.resize(ECommonTreeFactors::CTF_COUNT, 0);

    const TFactorNode* factorRoot = factorTree.GetRoot();

    ui32 rootDist = factorNode->MetaFactors.RootDist;
    factors[CTF_FACTOR_0] = rootDist;  // 1213
    factors[CTF_FACTOR_1] = DivideOrNaN(rootDist, factorRoot->MetaFactors.AvgLeavesDepth);  // 1214
    factors[CTF_FACTOR_2] = DivideOrNaN(rootDist, factorRoot->MetaFactors.MedLeavesDepth);  // 1215
    factors[CTF_FACTOR_3] = DivideOrNaN(rootDist, factorRoot->MetaFactors.MaxLeavesDepth);  // 1216

    factors[CTF_FACTOR_4] = factorRoot->MetaFactors.MaxLeavesBlockDepth;  // 1217
    factors[CTF_FACTOR_5] = factorRoot->MetaFactors.AvgLeavesBlockDepth;  // 1218

    factors[CTF_FACTOR_7] = factorNode->MetaFactors.MaxLeavesDepth;  // 1220
    factors[CTF_FACTOR_8] = factorNode->MetaFactors.AvgLeavesDepth;  // 1221
    factors[CTF_FACTOR_9] = factorNode->MetaFactors.MedLeavesDepth;  // 1222
    factors[CTF_FACTOR_10] = DivideOrNaN(rootDist, factorNode->MetaFactors.MaxLeavesBlockDepth);  // 1223
    factors[CTF_FACTOR_11] = DivideOrNaN(rootDist, factorNode->MetaFactors.MedLeavesBlockDepth);  // 1224
    factors[CTF_FACTOR_12] = DivideOrNaN(rootDist, factorNode->MetaFactors.AvgLeavesBlockDepth);  // 1225
    factors[CTF_FACTOR_13] = DivideOrNaN(rootDist, factorNode->MetaFactors.MaxLeavesDepth);  // 1226
    factors[CTF_FACTOR_14] = DivideOrNaN(rootDist, factorNode->MetaFactors.MedLeavesDepth);  // 1227
    factors[CTF_FACTOR_15] = DivideOrNaN(rootDist, factorNode->MetaFactors.AvgLeavesDepth);  // 1228

    ui32 tableDist = CalcParentTagDist(factorNode, HT_TABLE);
    factors[CTF_FACTOR_19] = (tableDist == BAD_ID) ? 0 : 1;  // 1232
    factors[CTF_FACTOR_20] = (tableDist == BAD_ID) ? nanf("") : tableDist;  // 1233
    factors[CTF_FACTOR_21] = (tableDist == BAD_ID) ? nanf("") : rootDist - tableDist;  // 1234
    ui32 pDist = CalcParentTagDist(factorNode, HT_P);
    factors[CTF_FACTOR_24] = (pDist == BAD_ID) ? nanf("") : rootDist - pDist;  // 1237
    ui32 formDist = CalcParentTagDist(factorNode, HT_FORM);
    factors[CTF_FACTOR_25] = (formDist == BAD_ID) ? 0 : 1;  // 1238
    factors[CTF_FACTOR_27] = (formDist == BAD_ID) ? nanf("") : rootDist - formDist;  // 1240
    ui32 sectionDist = CalcParentTagDist(factorNode, HT_SECTION);
    factors[CTF_FACTOR_50] = (sectionDist == BAD_ID) ? nanf("") : sectionDist;  // 1263
    ui32 ulDist = CalcParentTagDist(factorNode, HT_UL);
    factors[CTF_FACTOR_54] = (ulDist == BAD_ID) ? nanf("") : rootDist - sectionDist;  // 1267
    ui32 olDist = CalcParentTagDist(factorNode, HT_OL);
    factors[CTF_FACTOR_56] = (olDist == BAD_ID) ? nanf("") : olDist;  // 1269

    ui32 siblingsCount = 0;
    ui32 blockSiblingsCount = 0;
    ui32 nonBlockSiblingsCount = 0;
    TSiblingIterator siblingIterator(factorNode);
    for (const TFactorNode& sibling : siblingIterator) {
        ++siblingsCount;
        if (sibling.Node->IsElement()) {
            const NHtml::TTag& tag = NHtml::FindTag(sibling.Node->Tag());
            if (!IsInlineTag(tag)) {
                ++blockSiblingsCount;
            } else {
                ++nonBlockSiblingsCount;
            }
        }
    }
    factors[CTF_FACTOR_91] = siblingsCount;  // 1304
    factors[CTF_FACTOR_92] = blockSiblingsCount;  // 1305
    factors[CTF_FACTOR_93] = DivideOrNaN(blockSiblingsCount, siblingsCount);  // 1306
    factors[CTF_FACTOR_95] = DivideOrNaN(nonBlockSiblingsCount, siblingsCount);  // 1308
    ui32 initNodeIndex = siblingIterator.GetInitNodeIndex();
    Y_ASSERT(initNodeIndex != BAD_ID);
    factors[CTF_FACTOR_98] = float(initNodeIndex) / (siblingsCount + 1);  // 1311
    factors[CTF_FACTOR_99] = (initNodeIndex == 0) ? 1 : 0;  // 1312
    factors[CTF_FACTOR_100] = (initNodeIndex == siblingsCount) ? 1 : 0;  // 1313
    ui32 nodeTextLength = factorNode->Node->NodeText()->RawTextNormal().size();
    TVector<ui32> siblingsTextLength;
    for (TFactorNode& sibling : TSiblingIterator(factorNode)) {
        siblingsTextLength.push_back(sibling.Node->NodeText()->RawTextNormal().size());
    }
    siblingsTextLength.push_back(nodeTextLength);
    factors[CTF_FACTOR_101] = DivideOrNaN(nodeTextLength, CalcAvg(siblingsTextLength));  // 1314
    factors[CTF_FACTOR_102] = DivideOrNaN(nodeTextLength, CalcMiddle(siblingsTextLength));  // 1315
    size_t parentTextSize = (nullptr == factorNode->Parent) ? nodeTextLength :
        factorNode->Parent->Node->NodeText()->RawTextNormal().size();
    factors[CTF_FACTOR_103] = DivideOrNaN(nodeTextLength, parentTextSize); // 1316
    factors[CTF_FACTOR_132] = CalcSiblingsCountTag(factorNode, HT_P);  // 1345
    factors[CTF_FACTOR_133] = DivideOrNaN(CalcSiblingsCountTag(factorNode, HT_P), siblingsCount);  // 1346

    ui32 childrenCount = 0;
    ui32 textChildrenCount = 0;
    ui32 nonBlockChildrenCount = 0;
    for (TFactorNode& child : TNonEmptyChildIterator(factorNode)) {
        ++childrenCount;
        if (child.Node->IsElement()) {
            const NHtml::TTag& tag = NHtml::FindTag(child.Node->Tag());
            if (IsInlineTag(tag)) {
                ++nonBlockChildrenCount;
            }
        } else if (child.Node->IsText()) {
            if (!child.IsEmpty()) {
                ++textChildrenCount;
            }
        }
    }
    factors[CTF_FACTOR_152] = nonBlockChildrenCount;  // 1365
    factors[CTF_FACTOR_153] = DivideOrNaN(nonBlockChildrenCount, childrenCount);  // 1366
    factors[CTF_FACTOR_155] = DivideOrNaN(textChildrenCount, childrenCount);  // 1368

    TVector<ui32> siblingsChildrenCount;
    for (TFactorNode& sibling : TSiblingIterator(factorNode)) {
        ui32 siblingChildrenCount = 0;
        for (const TFactorNode& child : TNonEmptyChildIterator(&sibling)) {
            (void)child;
            ++siblingChildrenCount;
        }
        siblingsChildrenCount.push_back(siblingChildrenCount);
    }
    siblingsChildrenCount.push_back(childrenCount);
    factors[CTF_FACTOR_156] = DivideOrNaN(childrenCount, CalcAvg(siblingsChildrenCount));  // 1369
    factors[CTF_FACTOR_158] = DivideOrNaN(childrenCount, CalcMax(siblingsChildrenCount));  // 1371

    TVector<ui32> childrenChildrenCount;
    for (TFactorNode& child : TNonEmptyChildIterator(factorNode)) {
        ui32 childChildrenCount = 0;
        for (const TFactorNode& grandChild : TNonEmptyChildIterator(&child)) {
            (void)grandChild;
            ++childChildrenCount;
        }
        childrenChildrenCount.push_back(childChildrenCount);
    }
    factors[CTF_FACTOR_162] = childrenChildrenCount.empty() ? nanf("") : CalcVar(childrenChildrenCount);  // 1375

    factors[CTF_FACTOR_186] = CalcGrandChildrenCountTag(factorNode, HT_IMG);  // 1399

    ui32 subtreeBlockCount = 0;
    for (const TFactorNode& subtreeNode : TSubtreeIterator(factorNode, factorTree, false)) {
        if (subtreeNode.Node->IsElement()) {
            const NHtml::TTag& tag = NHtml::FindTag(subtreeNode.Node->Tag());
            if (!IsInlineTag(tag)) {
                ++subtreeBlockCount;
            }
        }
    }
    factors[CTF_FACTOR_255] = DivideOrNaN(CalcSubtreeCountTag(factorNode, factorTree, HT_IMG, false), subtreeBlockCount);  // 1468
    factors[CTF_FACTOR_270] = DivideOrNaN(CalcSubtreeCountTag(factorNode, factorTree, HT_FORM, false), subtreeBlockCount);  // 1483
    factors[CTF_FACTOR_271] = DivideOrNaN(CalcSubtreeCountTag(factorNode, factorTree, HT_INPUT, false), subtreeBlockCount);  // 1484

    TVector<ui32> childrenTextLengths;
    for (const TFactorNode& child : TNonEmptyChildIterator(factorNode)) {
        childrenTextLengths.push_back(child.Node->NodeText()->RawTextNormal().size());
    }
    factors[CTF_FACTOR_278] = childrenTextLengths.empty() ? nanf("") : CalcVar(childrenTextLengths);  // 1491

    ui32 subtreeNodesCount = 0;
    for (TFactorNode& subtreeNode : TSubtreeIterator(factorNode, factorTree)) {
        if (!subtreeNode.IsEmpty()) {
            ++subtreeNodesCount;
        }
    }
    factors[CTF_FACTOR_279] = subtreeNodesCount;  // 1492
    factors[CTF_FACTOR_323] = DivideOrNaN(CalcSubtreeCountTag(factorNode, factorTree, HT_P), subtreeNodesCount);  // 1536
}

}  // NSegmentator
