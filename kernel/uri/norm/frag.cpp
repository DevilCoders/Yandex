/*
 *  Created on: Feb 1, 2012
 *      Author: albert@
 *
 * $Id$
 */

#include "frag.h"

namespace Nydx {
namespace NUriNorm {

TFragNormalizer::TFragNormalizer(::NUri::TUri &uri, const TFlags &flags)
    : TdBase(uri, flags.GetNormalizeFrag())
    , Val_(GetField())
    , Change_(Val_.IsInited())
{
    do {
        if (!Change_)
            break;
        // #! ajax fragment is not insignificant
        if (!Val_.empty() && '!' == Val_[0]) {
            Keep();
            break;
        }
        Val_ = TStringBuf();
    }
    while (false);
}

}
}
