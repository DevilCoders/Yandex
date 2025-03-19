#pragma once

/*
 *  Created on: Jan 28, 2012
 *      Author: albert@
 *
 * $Id$
 */


#include "part.h"

namespace Nydx {
namespace NUriNorm {
namespace NTwit {

/**
 * Normalizes the Twitter URL.
 */
class THostSpec
{
public:
    THostSpec() = default;
    bool Normalize(const TStringBuf &hpref, TPartNormalizers &norm) const;
};

}
}
}
