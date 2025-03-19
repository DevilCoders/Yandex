#pragma once

/*
 *  Created on: May 25, 2010
 *      Author: albert@
 *
 * $Id$
 */


#include "normalizer.h"

namespace Nydx {
namespace NUriNorm {

class TUriNormalizerContainer
{
private:
    TUriNormalizer *Normalizer_;
    TUriNormalizer NormalizerDefault_;

public:
    TUriNormalizerContainer()
        : Normalizer_(NULL)
    {}

public:
    void SetUriNormalizer(TUriNormalizer &hn)
    {
        Normalizer_ = &hn;
    }
    TUriNormalizer &GetUriNormalizer()
    {
        return NULL != Normalizer_ ? *Normalizer_ : NormalizerDefault_;
    }
    const TUriNormalizer &GetUriNormalizer() const
    {
        return NULL != Normalizer_ ? *Normalizer_ : NormalizerDefault_;
    }
};

}

typedef NUriNorm::TUriNormalizerContainer TUriNormalizerContainer;

}
