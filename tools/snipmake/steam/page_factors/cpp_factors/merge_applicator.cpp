#include "merge_applicator.h"

#include "common_factors.h"
#include "factor_tree.h"
#include "merge_factors.h"

#include <kernel/matrixnet/mn_sse.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/string/split.h>


namespace NSegmentator {

void TMergeApplicator::Apply(bool useModel) {
    CalcFactors();
    if (useModel) {
        ApplyModel();
    }
}

void TMergeApplicator::ApplyModel() {
    TVector<bool> mergeResults;
    ui32 prevMergeCount = 0;
    for (const TFactorNode& node : TMergeNodeIterator(FactorTree)) {
        CorrectFactors(node.MergeFactors.Get(), prevMergeCount);
        double result = MnMerge.DoCalcRelev(node.MergeFactors->data());
        mergeResults.push_back(NeedToMerge(result));
        if (mergeResults.back()) {
            ++prevMergeCount;
        } else {
            prevMergeCount = 0;
        }
    }

    SetMergeMarks(mergeResults);
}

void TMergeApplicator::CalcFactors() {
    // prevMergeCount isn't included
    // MergeFactors for cur node: if cur node is needed to merge to prev node
    TUnsplittableNodeTraverser unsplittableNodes(FactorTree);
    TFactorNode* node = unsplittableNodes.Next();
    Y_ASSERT(nullptr != node);
    TFactors prevNodeFactors;
    CalcNodeFactors(prevNodeFactors, node);
    node = unsplittableNodes.Next();
    TFactors curNodeFactors;
    TFactors topNodeFactors;
    while (nullptr != node) {
        CalcNodeFactors(curNodeFactors, node);
        CalcTopNodeFactors(topNodeFactors, curNodeFactors, prevNodeFactors);
        node->MergeFactors.Reset(new TFactors());
        CalcNodeExtFactors(*node->MergeFactors, topNodeFactors);
        prevNodeFactors = curNodeFactors;
        node = unsplittableNodes.Next();
    }
}

void TMergeApplicator::SetMergeMarks(const TVector<bool>& mergeResults) {
    ui32 curMergeId = 0;
    TVector<bool>::const_iterator mergeResult = mergeResults.begin();

    TUnsplittableNodeTraverser unsplittableNodes(FactorTree);
    TFactorNode* node = unsplittableNodes.Next();
    node->MergeId = curMergeId;
    node = unsplittableNodes.Next();
    while (nullptr != node) {
        if (!(*mergeResult)) {
            ++curMergeId;
        }
        node->MergeId = curMergeId;
        ++mergeResult;
        node = unsplittableNodes.Next();
    }
}

static const size_t CHAR_FACTORS_OFFSET = 50;
static const size_t TREE_FACTORS_OFFSET = 1213;
void TMergeApplicator::CalcNodeFactors(TFactors& factors, TFactorNode* factorNode) const {
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

void TMergeApplicator::CalcTopNodeFactors(TFactors& factors, const TFactors& curNodeFactors, const TFactors& prevNodeFactors) const {
    factors.clear();
    factors.resize(EMergeFactors::MF_COUNT, 0);

    /*
     * see https://beta.wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/Projects/Segmentator/factors/
     *
     * switch (formulaFactorId % 3):
     *     case 0: prevNodeFactor
     *     case 1: curNodeFactor
     *     case 2: abs(curNodeFactor - prevNodeFactor)
     */

    factors[MF_FACTOR_7] = Abs(curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_2] - prevNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_2]);
    factors[MF_FACTOR_8] = curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_9];
    factors[MF_FACTOR_9] = curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_28];
    factors[MF_FACTOR_10] = curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_48];
    factors[MF_FACTOR_11] = curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_52];
    factors[MF_FACTOR_12] = prevNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_68];
    factors[MF_FACTOR_13] = Abs(curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_68] - prevNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_68]);
    factors[MF_FACTOR_14] = Abs(curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_69] - prevNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_69]);
    factors[MF_FACTOR_15] = curNodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_138];

    factors[MF_FACTOR_53] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_1];
    factors[MF_FACTOR_54] = Abs(curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_1] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_1]);
    factors[MF_FACTOR_55] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_2];
    factors[MF_FACTOR_56] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_2];
    factors[MF_FACTOR_57] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_3];
    factors[MF_FACTOR_58] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_3];
    factors[MF_FACTOR_59] = Abs(curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_3] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_3]);
    factors[MF_FACTOR_60] = Abs(curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_13] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_13]);
    factors[MF_FACTOR_61] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_14];
    factors[MF_FACTOR_62] = Abs(curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_14] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_14]);
    factors[MF_FACTOR_63] = Abs(curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15]);
    factors[MF_FACTOR_64] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_20];
    factors[MF_FACTOR_65] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_24];
    factors[MF_FACTOR_66] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_24];
    factors[MF_FACTOR_67] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_25] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_25];
    factors[MF_FACTOR_68] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_91];
    factors[MF_FACTOR_69] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_91];
    factors[MF_FACTOR_70] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_92];
    factors[MF_FACTOR_71] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_93];
    factors[MF_FACTOR_72] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_95];
    factors[MF_FACTOR_73] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98];
    factors[MF_FACTOR_74] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98];
    factors[MF_FACTOR_75] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98];
    factors[MF_FACTOR_76] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_100] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_100];
    factors[MF_FACTOR_77] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_101];
    factors[MF_FACTOR_78] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_102];
    factors[MF_FACTOR_79] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_103];
    factors[MF_FACTOR_80] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_103];
    factors[MF_FACTOR_81] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_103] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_103];
    factors[MF_FACTOR_82] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_132];
    factors[MF_FACTOR_83] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_132] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_132];
    factors[MF_FACTOR_84] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_133];
    factors[MF_FACTOR_85] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_133] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_133];
    factors[MF_FACTOR_86] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_152] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_152];
    factors[MF_FACTOR_87] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_153];
    factors[MF_FACTOR_88] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_155] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_155];
    factors[MF_FACTOR_89] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_156];
    factors[MF_FACTOR_90] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_158];
    factors[MF_FACTOR_91] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_186] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_186];
    factors[MF_FACTOR_92] = prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_255];
    factors[MF_FACTOR_93] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_255];
    factors[MF_FACTOR_94] = curNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_323] - prevNodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_323];
}

void TMergeApplicator::CalcNodeExtFactors(TFactors& nodeExtFactors, const TFactors& factors) const {
    TSet<ui32> factorIds;
    MnMerge.UsedFactors(factorIds);
    CalcExtFactors(nodeExtFactors, factors, factorIds, MnMerge.GetNumFeats(), EMergeFactors::MF_COUNT);
}

void TMergeApplicator::CorrectFactors(TFactors* segmentFactors, ui32 prevMergeCount) {
    segmentFactors->back() = prevMergeCount;
}

bool TMergeApplicator::NeedToMerge(double mergeResult) const {
    return mergeResult > MergeResultBorder;
}


// TAwareMergeApplicator
void TAwareMergeApplicator::ApplyModel() {
    const NJson::TJsonValue::TArray& mergeArray = CorrectSegmentation["merge"].GetArraySafe();
    const NJson::TJsonValue::TArray& choicesArray = CorrectSegmentation["choice"].GetArraySafe();
    if (choicesArray.empty()) {
        ythrow yexception() << "empty choices";
    }

    TVector<ui32> mergeStartIds;
    // the last elem is a fake startId
    mergeStartIds.reserve(mergeArray.size() + 1);
    mergeStartIds.push_back(0);
    for (const NJson::TJsonValue& mergeInfoJson : mergeArray) {
        const TString& mergeInfoStr = mergeInfoJson.GetStringSafe();
        TVector<TString> splitMergeInfo;
        StringSplitter(mergeInfoStr).Split(',').SkipEmpty().Collect(&splitMergeInfo);
        ui32 startId, endId;
        if (splitMergeInfo.size() != 3 ||
            !TryFromString<ui32>(splitMergeInfo[0], startId) || !TryFromString<ui32>(splitMergeInfo[1], endId) ||
            startId != mergeStartIds.back() || startId >= endId)
        {
            ythrow yexception() << "invalid merge info format: '" << mergeInfoStr << "'";
        }
        mergeStartIds.push_back(endId);
    }
    if (mergeStartIds.back() != choicesArray.size()) {
        ythrow yexception() << "choices size doesn't match merge info: "
                << mergeStartIds.back() << " != " << choicesArray.size();
    }

    using THMapSplitGuids2Id = THashMap<TSplitGuid, TVector<ui32>, TSplitGuidHash>;
    THMapSplitGuids2Id splitGuids2MergeId;
    ui32 choiceId = 0;
    ui32 mergeId = 0;
    for (const NJson::TJsonValue& guidJson : choicesArray) {
        if (choiceId >= mergeStartIds[mergeId + 1]) {
            ++mergeId;
        }
        const TString& guid = guidJson.GetStringSafe();
        TSplitGuid splitGuid(guid);

        THMapSplitGuids2Id::iterator curSplitGuidIt = splitGuids2MergeId.find(splitGuid);
        if (curSplitGuidIt != splitGuids2MergeId.end()) {
            curSplitGuidIt->second.push_back(mergeId);
        } else {
            splitGuids2MergeId.insert(THMapSplitGuids2Id::value_type(splitGuid, TVector<ui32>(1, mergeId)));
        }

        ++choiceId;
    }

    ui32 prevMergeCount = 0;
    mergeId = 0;

    TUnsplittableNodeTraverser unsplittableNodes(FactorTree);
    TFactorNode* node = unsplittableNodes.Next();
    Y_ASSERT(nullptr != node);
    node->MergeId = mergeId;
    node = unsplittableNodes.Next();
    while (nullptr != node) {
        CorrectFactors(node->MergeFactors.Get(), prevMergeCount);
        TStringBuf guid = node->GetGuid();

        THMapSplitGuids2Id::iterator splitGuidIt = splitGuids2MergeId.find(TSplitGuid(TString(guid.data(), guid.size())));
        if (splitGuidIt == splitGuids2MergeId.end() && nullptr != node->Parent) {
            if (BAD_ID != node->MetaFactors.FakeGuidIdx) {
                Cerr << "INFO: guid '" << guid << "' not found, try parent's guid\n";
                guid = node->Parent->GetGuid();
                splitGuidIt = splitGuids2MergeId.find(TSplitGuid(TString(guid.data(), guid.size())));
            }
        }

        Cerr << "INFO: selected guid: '" << guid << "'\n";
        if (splitGuidIt != splitGuids2MergeId.end()) {
            if (splitGuidIt->second[0] == mergeId) {
                Cerr << "INFO: OK, merge";
                ++prevMergeCount;
            } else if (splitGuidIt->second[0] == mergeId + 1) {
                Cerr << "INFO: OK, no merge";
                ++mergeId;
                prevMergeCount = 0;
            } else {
                // TODO: smth wrong with nodes
                Cerr << "WARNING: smth wrong with nodes: merge ids " << mergeId
                    << " -> " << splitGuidIt->second[0];
                mergeId = splitGuidIt->second[0];
                prevMergeCount = 0;
            }
            splitGuidIt->second.erase(splitGuidIt->second.begin());
            if (splitGuidIt->second.empty()) {
                splitGuids2MergeId.erase(splitGuidIt);
            }
        } else {
            // TODO: not found
            Cerr << "WARNING: not found";
        }
        node->MergeId = mergeId;

        node = unsplittableNodes.Next();
        Cerr << Endl;
    }

    if (splitGuids2MergeId.empty()) {
        return;
    }
    Cerr << "WARNING: not used answers: {";
    TSet<TSplitGuid> sortedSplitGuids;
    for (const THMapSplitGuids2Id::value_type& splitGuid2MergeId : splitGuids2MergeId) {
        sortedSplitGuids.insert(splitGuid2MergeId.first);
    }
    for (const TSplitGuid& splitGuid : sortedSplitGuids) {
        Cerr << splitGuid.Guid << " (" << (splitGuid.IsNative ? "native" : "fake") << "): [";
        for (ui32 splitMergeId : splitGuids2MergeId[splitGuid]) {
            Cerr << splitMergeId << ", ";
        }
        Cerr << "], ";
    }
    Cerr << "}" << Endl;
}

}  // NSegmentator
