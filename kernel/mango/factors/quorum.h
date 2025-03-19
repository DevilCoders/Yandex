#pragma once

#include <cmath>
#include <util/generic/singleton.h>
#include <util/generic/algorithm.h>

namespace NMango
{

// quorum - копипаст из базового поиска - TODO: надо как-то решить вопрос с Деном ********************
static const float WEIGHT0 = 0.995f;

struct TWebDocQuorum {
    float Weights[100];

    TWebDocQuorum(float mul = 1.0f);

    inline float Get(int wordCount) const {
        if (wordCount > 0) {
            return Weights[Min(wordCount - 1, 99)];
        } else {
            return 1.0;
        }
    }
};


inline float IdfToQuorumRf(float idf)
{
    return exp(idf * 0.38f);
}


} // NMango
