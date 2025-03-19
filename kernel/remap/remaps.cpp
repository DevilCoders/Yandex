#include <cmath>

#include <algorithm>

#include <util/generic/yexception.h>
#include <library/cpp/deprecated/split/split_iterator.h>

#include "remap_table.h"
#include "remaps.h"

using namespace std;

const float eps = 1E-7f;

void GetExpGroupIndices(size_t count, size_t groupCount, size_t groupGrowFactor, TVector<size_t>* indices) {
    TVector<size_t> bucketSize;
    bucketSize.push_back(1);
    size_t bucketSizeSum = 1;
    for (size_t i = 1; i < groupCount; ++i) {
        bucketSize.push_back(bucketSize.back() * groupGrowFactor);
        bucketSizeSum += bucketSize.back();
    }

    int ratio = static_cast<int>(floor(static_cast<float>(count / bucketSizeSum)) + eps);
    if (ratio == 0) {
        ratio = 1;
    }

    indices->clear();

    size_t itemIndex = count - 1;

    indices->push_back(itemIndex);

    for (size_t i = 0; i < groupCount - 1; ++i) {
        size_t realBucketSize = bucketSize[i] * ratio;
        if (realBucketSize > itemIndex) {
            break;
        }
        itemIndex -= realBucketSize;
        indices->push_back(itemIndex);
    }

    indices->push_back(0);

    reverse(indices->begin(), indices->end());
}

TRemapTable GetRemapExp(const TVector<float> &data, size_t groupCount, size_t groupGrowFactor) {
    TVector<float> sortedData(data.begin(), data.end());
    sort(sortedData.begin(), sortedData.end());

    TVector<size_t> indices;

    GetExpGroupIndices(data.size(), groupCount, groupGrowFactor, &indices);
    TVector<float> remapData;
    for (TVector<size_t>::const_iterator it = indices.begin(); it != indices.end(); ++it)
        remapData.push_back(sortedData[*it]);

    TRemapTable remapTable(remapData.data(), (int)remapData.size());
    return remapTable;
}

void DoLinearRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues) {
    if (allValues.size()) {
        float maxValue = *max_element(allValues.begin(), allValues.end());
        for (TVector<float*>::const_iterator it = remappingValues.begin(); it != remappingValues.end(); ++it)
            **it = **it / maxValue;
    }
}

void DoGroupRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues, size_t groupCount) {
    TVector<float> allValuesCopy(allValues.begin(), allValues.end());

    sort(allValuesCopy.begin(), allValuesCopy.end());
    TVector<float> remapData;
    {
        size_t step = allValuesCopy.size() / groupCount;
        for (size_t i = 0; i < groupCount; ++i)
            remapData.push_back(allValuesCopy[step * i]);
        remapData.push_back(allValuesCopy.back());
    }
    TRemapTable remap(remapData.data(), (int)remapData.size());

    for (TVector<float*>::const_iterator it = remappingValues.begin(); it != remappingValues.end(); ++it)
        **it = static_cast<float>(remap.Remap(**it));
}

void DoExpRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues, size_t groupCount, size_t groupGrowFactor) {
    TRemapTable remap = GetRemapExp(allValues, groupCount, groupGrowFactor);
    for (TVector<float*>::const_iterator it = remappingValues.begin(); it != remappingValues.end(); ++it)
        **it  = static_cast<float>(remap.Remap(**it));
}

void DoLogRemap(const TVector<float> &allValues, const TVector<float*>& remappingValues) {
    float maxValue = log(*max_element(allValues.begin(), allValues.end()));
    for (TVector<float*>::const_iterator it = remappingValues.begin(); it != remappingValues.end(); ++it)
        **it = log(**it) / maxValue;
}

void TLinearRemap::Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) {
    return DoLinearRemap(allValues, remappingValues);
}

TGroupRemap::TGroupRemap(size_t groupCount)
    : GroupCount(groupCount)
{
}

void TGroupRemap::Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) {
    return DoGroupRemap(allValues, remappingValues, GroupCount);
}

TExpRemap::TExpRemap(size_t groupCount, size_t groupGrowFactor)
    : GroupCount(groupCount)
    , GroupGrowFactor(groupGrowFactor)
{
}

void TExpRemap::Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) {
    DoExpRemap(allValues, remappingValues, GroupCount, GroupGrowFactor);
}

void TLogRemap::Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) {
    DoLogRemap(allValues, remappingValues);
}

TOrderExpGroupRemap::TOrderExpGroupRemap(size_t groupCount, size_t groupGrowFactor)
    : GroupCount(groupCount)
    , GroupGrowFactor(groupGrowFactor)
{
}

// TODO may be can be use <inline> as earlier
float OrderRemap(const TVector<size_t>& indices, const TVector<float>& sortedData, float value) {
    const size_t rangeCount = indices.size() - 1;
    const float remappedRangeSize = float(1) / rangeCount;

    TVector<float>::const_iterator it = upper_bound(sortedData.begin(), sortedData.end(), value);

    //upper-or-equal bound (last of equals selected)
    if (it != sortedData.begin() && *(it - 1) == value) {
        --it;
    }

    size_t index = static_cast<size_t>(it - sortedData.begin());

    for (size_t range = 0; range < indices.size() - 1; ++range) {
        const size_t startIndex = indices[range];
        const size_t endIndex = indices[range + 1];

        if (index >= startIndex && index <= endIndex) {
            size_t rangeSize = endIndex - startIndex;
            if (rangeSize == 0 || rangeCount == 0)
                return NAN;
            return float(index - startIndex) / (rangeSize * rangeCount) + remappedRangeSize * range;
        }
    }

    return 1;
}

void TOrderExpGroupRemap::Remap(const TVector<float> &allValues, const TVector<float*>& remappingValues) {
    TVector<size_t> indices;
    GetExpGroupIndices(allValues.size(), GroupCount, GroupGrowFactor, &indices);

    TVector<float> sortedData(allValues.begin(), allValues.end());
    sort(sortedData.begin(), sortedData.end());

    for (TVector<float*>::const_iterator it = remappingValues.begin();
         it != remappingValues.end(); ++it) {

             **it = OrderRemap(indices, sortedData, **it);
    }
}

void TTrivialRemap::Remap(const TVector<float>&, const TVector<float*>&) {
}

TRemapPtr GetRemap(const TString& remapParameters) {
    TVector<TString> words;
    TSplitDelimiters delims("@");
    Split(TDelimitersStrictSplit(remapParameters, delims), &words);

    if (words.size() == 0) {
        return TRemapPtr(new TTrivialRemap());
    }

    const TString& remapType = words[0];
    if (remapType == "lin") {
        if (words.size() != 1) {
            ythrow yexception() << "Linear remap should not have any parameters, but '" <<  remapParameters.data() << "' was specified";
        }
        return TRemapPtr(new TLinearRemap());
    } else if (remapType == "grp") {
        if (words.size() != 2) {
            ythrow yexception() << "Group remap should have only one parameter - group count, but '" <<  remapParameters.data() << "' was specified";
        }
        size_t groupCount = FromString<size_t>(words[1]);
        return TRemapPtr(new TGroupRemap(groupCount));
    } else if (remapType == "exp") {
        if (words.size() != 3) {
            ythrow yexception() << "Exponential remap should have two parameters - group count and grow factor, but '" <<  remapParameters.data() << "' was specified";
        }
        size_t groupCount = FromString<size_t>(words[1]);
        size_t groupGrowFactor = FromString<size_t>(words[2]);
        return TRemapPtr(new TExpRemap(groupCount, groupGrowFactor));
    } else if (remapType == "log") {
        if (words.size() != 1) {
            ythrow yexception() << "Logarithmic remap should not have any parameters, but '" <<  remapParameters.data() << "' was specified";
        }
        return TRemapPtr(new TLogRemap());
    } else if (remapType == "orderExpGroup") {
        if (words.size() != 3) {
            ythrow yexception() << "Order Exponential Group remap should have two parameters - group count and grow factor, but '" <<  remapParameters.data() << "' was specified'";
        }
        size_t groupCount = FromString<size_t>(words[1]);
        size_t groupGrowFactor = FromString<size_t>(words[2]);
        return TRemapPtr(new TOrderExpGroupRemap(groupCount, groupGrowFactor));
    } else {
        ythrow yexception() << "Wrong remap parameters specified: '" <<  remapParameters.data() << "'";
    }
}

const char* remapsHelpStrings[] = {
    "lin - linear remap",
    "grp@<group count> - group remap",
    "exp@<group count>@<group grow> - exponential group remap",
    "log - logarithmic remap",
    "orderExpGroup@<group count>@<group grow> - exponential group order remap"
};

const size_t remapsHelpStringCount = sizeof(remapsHelpStrings) / sizeof(remapsHelpStrings[0]);

