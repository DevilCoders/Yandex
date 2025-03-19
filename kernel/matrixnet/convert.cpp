#include "convert.h"

#include <util/generic/utility.h>

namespace NMatrixnet {

static void MnConvert(const TMnSseStatic &from, TMnTrees &to, const TModelInfo &info) {
    TVector<ui32> condition2featureId(from.Meta.ValuesSize);
    {
        int condIdx = 0;
        for (size_t i = 0; i < from.Meta.FeaturesSize; i++) {
            for (ui32 j = 0; j < from.Meta.Features[i].Length; j++) {
                condition2featureId[condIdx++] = from.Meta.Features[i].Index;
            }
        }
    }
    int dataIdx = 0;
    int treeCondIdx = 0;
    TVector<double> dataBuf;
    TVector<TMnTree> trees;
    auto& fromData = std::get<TMultiData>(from.Leaves.Data).MultiData[0];
    for (size_t condNum = 0; condNum < from.Meta.SizeToCountSize; condNum++) {
        const int treesNum = from.Meta.SizeToCount[condNum];
        const int curDataSize = 1 << condNum;
        for (int treeIdx = 0; treeIdx < treesNum; treeIdx++) {
            TMnTree tree(condNum);
            for (size_t i = 0; i < condNum; i++) {
                const int condIdx = from.Meta.GetIndex(treeCondIdx++);
                tree.SetCondition(i, condition2featureId[condIdx], from.Meta.Values[condIdx]);
            }

            dataBuf.resize(curDataSize);

            for (int i = 0; i < curDataSize; i++) {
                const double val = double(ui32(fromData.Data[dataIdx++])) - (1L << 31);
                dataBuf[i] = val * fromData.Norm.DataScale;
            }
            tree.SetValues(dataBuf.data());
            trees.push_back(tree);
        }
    }

    DoSwap(to.Trees, trees);
    to.Info.clear();
    to.Info.insert(info.begin(), info.end());
    to.Bias = fromData.Norm.DataBias;
}

void MnConvert(const TMnSseInfo &from, TMnTrees &to) {
    MnConvert(from.GetSseDataPtrs(), to, from.Info);
}

void MnConvert(const TMnSseDynamic &from, TMnTrees &to) {
    MnConvert(from.GetSseDataPtrs(), to, from.Info);
}

}

