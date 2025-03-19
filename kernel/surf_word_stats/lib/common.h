#pragma once

#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/vector.h>

#include <algorithm>

namespace NWordFeatures {

    const static size_t MaxKeyLen = 80;

    enum ESurfFeatureType {
        SURF_DEPTH_NODES_GRADIENT = 0  /* "SURF_DEPTH_NODES_GRADIENT" */,
        SURF_DEPTH_NODES_GRADIENT_DISP /* "SURF_DEPTH_NODES_GRADIENT_DISP" */,
        SURF_DEPTH_NODES_LEAF_LENGTH /* "SURF_DEPTH_NODES_LEAF_LENGTH" */,
        SURF_DEPTH_NODES_LEAF_LENGTH90 /* "SURF_DEPTH_NODES_LEAF_LENGTH90" */,
        SURF_DEPTH_TIME_GRADIENT /* "SURF_DEPTH_TIME_GRADIENT" */,
        SURF_DEPTH_TIME_GRADIENT_DISP /* "SURF_DEPTH_TIME_GRADIENT_DISP" */,
        SURF_DEPTH_TIME_LEAF_LENGTH /* "SURF_DEPTH_TIME_LEAF_LENGTH" */,
        SURF_DEPTH_TIME_LEAF_LENGTH90 /* "SURF_DEPTH_TIME_LEAF_LENGTH90" */,
        SURF_NODES_TIME_GRADIENT /* "SURF_NODES_TIME_GRADIENT" */,
        SURF_NODES_TIME_GRADIENT_DISP /* "SURF_NODES_TIME_GRADIENT_DISP" */,
        SURF_NODES_TIME_LEAF_LENGTH /* "SURF_NODES_TIME_LEAF_LENGTH" */,
        SURF_NODES_TIME_LEAF_LENGTH90 /* "SURF_NODES_TIME_LEAF_LENGTH90" */,
        SURF_NODES_HANGS_GRADIENT /* "SURF_NODES_HANGS_GRADIENT" */,
        SURF_TIME /* "SURF_TIME" */,
        SURF_NODES /* "SURF_NODES" */,
        SURF_DEPTH /* "SURF_DEPTH" */,
        SURF_HANGS /* "SURF_HANGS" */,
        SURF_DOWNLOADS /* "SURF_DOWNLOADS" */,

        SURF_TOTAL,
    };

    enum ESurfStatisticType {
        SURF_STAT_MEAN = 0,
        SURF_STAT_MIN,
        SURF_STAT_MAX,

        SURF_STAT_TOTAL
    };

    struct TSurfFeatures {
        float Features[SURF_TOTAL];

        TSurfFeatures()
        {
            Clear();
        }

        void Clear()
        {
            std::fill(Features, Features + SURF_TOTAL, 0.0f);
        }

        TSurfFeatures& operator+=(const TSurfFeatures& other)
        {
            for (size_t i = 0; i < SURF_TOTAL; ++i) {
                Features[i] += other.Features[i];
            }
            return *this;
        }

        TSurfFeatures& operator*=(double f)
        {
            for (size_t i = 0; i < SURF_TOTAL; ++i) {
                Features[i] *= f;
            }
            return *this;
        }
    };

}; // NWordFeatures

template<>
inline NWordFeatures::TSurfFeatures FromString<NWordFeatures::TSurfFeatures>(const char* data, size_t len)
{
    NWordFeatures::TSurfFeatures result;

    TStringBuf buf(data, len);

    for (size_t i = 0; i < NWordFeatures::SURF_TOTAL; ++i) {
        result.Features[i] = FromString<float>(buf.NextTok('\t'));
    }

    return result;
}

template<>
inline TString ToString<NWordFeatures::TSurfFeatures>(const NWordFeatures::TSurfFeatures& data)
{
    return JoinStrings(data.Features, data.Features + NWordFeatures::SURF_TOTAL, "\t");
}
