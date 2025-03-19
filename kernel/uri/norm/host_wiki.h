#pragma once

/*
 *  Created on: Dec 7, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include "part.h"

namespace Nydx {
namespace NUriNorm {
namespace NWiki {

/**
 * Normalizes the Wikipedia path.
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
