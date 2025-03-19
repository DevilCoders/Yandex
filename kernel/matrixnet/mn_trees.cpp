#include "mn_trees.h"

#include <algorithm>

#include <util/generic/yexception.h>

namespace NMatrixnet {

TMnTree::TMnTree(const int size)
    : Features(size, 0)
    , Borders(size, 0.0f)
    , Values(1 << size, 0.0f)
{
}

void TMnTree::SetCondition(const int idx, const ui32 featureId, const float border) {
    Features[idx] = featureId;
    Borders[idx] = border;
}

void TMnTree::SetValues(const double* values) {
    Values.assign(values, values + (size_t(1) << Features.ysize()));
}

size_t TMnTree::GetNumFeats() const {
    return (size_t) *std::max_element(Features.begin(), Features.end()) + 1;
}

size_t TMnTrees::GetNumFeats() const {
    size_t result = 0;
    for (int i = 0; i < Trees.ysize(); ++i) {
        result = Max(result, Trees[i].GetNumFeats());
    }
    return result;
}

double TMnTrees::DoCalcRelev(const float* features) const {
    double result = 0.0;
    for (int i = 0; i < Trees.ysize(); ++i) {
        result += Trees[i].DoCalcRelev(features);
    }
    return result + Bias;
}

}

