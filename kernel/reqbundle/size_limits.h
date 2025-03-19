#pragma once

namespace NReqBundle {
    struct TSizeLimits {
        enum {
            MaxNumFacets = 1000,
            MaxNumMatches = 10000,
            MaxNumConstraints = 100,
            MaxNumWordsInRequest = 1000,
            MaxNumRequests = 10000,

            MaxNumForms = 10000,
            MaxNumLemmas = 10000,
            MaxNumWordsInBlock = 1000,
            MaxNumBlocks = 4000, // iterator can't handle more that 4096

            MaxMainPartsWordMasks = MaxNumMatches,
            MaxMarkupPartsBestFormClasses = MaxNumMatches,
        };
    };
} // NReqBundle
