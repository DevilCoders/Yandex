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
namespace NAmzn {

/**
 * Normalizes the Amazon path.
 */
class THostSpec
    : public TNonCopyable
{
    class TImpl;
    TImpl *Impl_;

public:
    THostSpec();
    ~THostSpec();
    bool Normalize(TPartNormalizers &norm) const;
};

}
}
}
