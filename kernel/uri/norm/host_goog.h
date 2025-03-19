#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include "part.h"

#include <util/generic/ptr.h>

namespace Nydx {
namespace NUriNorm {
namespace NGoog {

/**
 * Normalizes the Google path.
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
