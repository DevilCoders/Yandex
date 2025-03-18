#include "annotate_applicator.h"

#include "annotate_factors.h"
#include "common_factors.h"
#include "factor_tree.h"

#include <kernel/matrixnet/mn_multi_categ.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/algorithm.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/vector.h>
#include <util/string/split.h>

namespace NSegmentator {

void TAnnotateApplicator::Apply(bool useModel) {
    CalcFactors();
    if (useModel) {
        ApplyModel();
    }
}

void TAnnotateApplicator::ApplyModel() {
    TVector<TFactors> annotateFactorsByVals;
    for (const TFactorNode& node : TAnnotateNodeIterator(FactorTree)) {
        annotateFactorsByVals.push_back(*node.AnnotateFactors.Get());
    }
    double annotateResults[annotateFactorsByVals.size() * MnAnnotate.CategValues().size()];
    MnAnnotate.CalcCategs(annotateFactorsByVals, annotateResults);

    SetTypeMarks(annotateResults);
}

void TAnnotateApplicator::CalcFactors() {
    // TODO: aggregate factors from segment nodes
    TMergeSegmentTraverser mergeSegments(FactorTree);
    TPageSegment seg = mergeSegments.Next();
    TFactors nodeFactors;
    TFactors topNodeFactors;
    while (!seg.Empty()) {
        CalcNodeFactors(nodeFactors, seg.First);
        CalcTopNodeFactors(topNodeFactors, nodeFactors);
        seg.First->AnnotateFactors.Reset(new TFactors());
        CalcNodeExtFactors(*seg.First->AnnotateFactors, topNodeFactors);
        seg = mergeSegments.Next();
    }
}

void TAnnotateApplicator::SetTypeMarks(const double* annotateResults) {
    auto categValuesRef = MnAnnotate.CategValues();
    const double* offset = annotateResults;
    for (TFactorNode& node : TAnnotateNodeIterator(FactorTree)) {
        node.TypeId = CalcSegmentTypeId(offset, categValuesRef);
        offset += categValuesRef.size();
    }
}

static const size_t CHAR_FACTORS_OFFSET = 50;
static const size_t TREE_FACTORS_OFFSET = 1213;
void TAnnotateApplicator::CalcNodeFactors(TFactors& factors, TFactorNode* factorNode) const {
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

void TAnnotateApplicator::CalcTopNodeFactors(TFactors& factors, const TFactors& nodeFactors) const {
    factors.clear();
    factors.resize(EAnnotateFactors::AF_COUNT, 0);

    factors[AF_FACTOR_6] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_0];
    factors[AF_FACTOR_7] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_5];
    factors[AF_FACTOR_8] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_6];
    factors[AF_FACTOR_9] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_7];
    factors[AF_FACTOR_10] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_24];
    factors[AF_FACTOR_11] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_26];
    factors[AF_FACTOR_12] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_27];
    factors[AF_FACTOR_13] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_28];
    factors[AF_FACTOR_14] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_29];
    factors[AF_FACTOR_15] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_57];
    factors[AF_FACTOR_16] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_60];
    factors[AF_FACTOR_17] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_62];
    factors[AF_FACTOR_18] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_65];
    factors[AF_FACTOR_19] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_69];
    factors[AF_FACTOR_20] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_71];
    factors[AF_FACTOR_21] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_76];
    factors[AF_FACTOR_22] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_78];
    factors[AF_FACTOR_23] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_79];
    factors[AF_FACTOR_24] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_136];
    factors[AF_FACTOR_25] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_138];
    factors[AF_FACTOR_26] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_154];
    factors[AF_FACTOR_27] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_170];
    factors[AF_FACTOR_28] = nodeFactors[CHAR_FACTORS_OFFSET + CCF_FACTOR_172];

    factors[AF_FACTOR_63] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_0];
    factors[AF_FACTOR_64] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_1];
    factors[AF_FACTOR_65] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_2];
    factors[AF_FACTOR_66] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_3];
    factors[AF_FACTOR_67] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_7];
    factors[AF_FACTOR_68] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_8];
    factors[AF_FACTOR_69] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_9];
    factors[AF_FACTOR_70] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_13];
    factors[AF_FACTOR_71] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_14];
    factors[AF_FACTOR_72] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_15];
    factors[AF_FACTOR_73] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_19];
    factors[AF_FACTOR_74] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_20];
    factors[AF_FACTOR_75] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_21];
    factors[AF_FACTOR_76] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_25];
    factors[AF_FACTOR_77] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_27];
    factors[AF_FACTOR_78] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_50];
    factors[AF_FACTOR_79] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_54];
    factors[AF_FACTOR_80] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_91];
    factors[AF_FACTOR_81] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_92];
    factors[AF_FACTOR_82] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_93];
    factors[AF_FACTOR_83] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_95];
    factors[AF_FACTOR_84] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_98];
    factors[AF_FACTOR_85] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_99];
    factors[AF_FACTOR_86] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_100];
    factors[AF_FACTOR_87] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_101];
    factors[AF_FACTOR_88] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_103];
    factors[AF_FACTOR_89] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_132];
    factors[AF_FACTOR_90] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_133];
    factors[AF_FACTOR_91] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_152];
    factors[AF_FACTOR_92] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_153];
    factors[AF_FACTOR_93] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_156];
    factors[AF_FACTOR_94] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_158];
    factors[AF_FACTOR_95] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_162];
    factors[AF_FACTOR_96] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_255];
    factors[AF_FACTOR_97] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_271];
    factors[AF_FACTOR_98] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_278];
    factors[AF_FACTOR_99] = nodeFactors[TREE_FACTORS_OFFSET + CTF_FACTOR_279];
}

void TAnnotateApplicator::CalcNodeExtFactors(TFactors& nodeExtFactors, const TFactors& factors) const {
    TSet<ui32> factorIds;
    MnAnnotate.UsedFactors(factorIds);
    CalcExtFactors(nodeExtFactors, factors, factorIds, MnAnnotate.GetNumFeats(), EAnnotateFactors::AF_COUNT);
}

ui32 TAnnotateApplicator::CalcSegmentTypeId(const double* result, TConstArrayRef<double> categValues) {
    size_t maxIndex = MaxElement(result, result + categValues.size()) - result;
    return static_cast<ui32>(categValues[maxIndex] + 0.5);
}


// TAwareAnnotateApplicator
void TAwareAnnotateApplicator::ApplyModel() {
    const NJson::TJsonValue::TArray& mergeArray = CorrectSegmentation["merge"].GetArraySafe();
    TVector<ui32> annotateTypes;
    annotateTypes.reserve(mergeArray.size());
    for (const NJson::TJsonValue& mergeInfoJson : mergeArray) {
        const TString& mergeInfoStr = mergeInfoJson.GetStringSafe();
        TVector<TString> splitMergeInfo;
        StringSplitter(mergeInfoStr).Split(',').SkipEmpty().Collect(&splitMergeInfo);
        if (splitMergeInfo.size() != 3) {
            ythrow yexception() << "invalid merge info format: '" << mergeInfoStr << "'";
        }
        const ui32* classCode = Classes2Codes.FindPtr(splitMergeInfo[2]);
        if (nullptr == classCode) {
            ythrow yexception() << "Unknown annotate class: '" << mergeInfoStr << "'";
        }
        annotateTypes.push_back(*classCode);
    }

    for (TFactorNode& node : TAnnotateNodeIterator(FactorTree)) {
        Y_ASSERT(node.MergeId < annotateTypes.size());
        node.TypeId = annotateTypes[node.MergeId];
    }
}

}  // NSegmentator
