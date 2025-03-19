/*
 *  Created on: Feb 1, 2012
 *      Author: albert@
 *
 * $Id$
 */

#include "part.h"

namespace Nydx {
namespace NUriNorm {

TPartNormalizers::TPartNormalizers(::NUri::TUri &uri
    , const TFlags &flags, const THostNormalizerBase &hostbase)
    : Uri_(uri)
    , Flags_(flags)
    , IsSet_(false)
    , Scheme(uri, flags)
    , Host(uri, flags, hostbase)
    , Path(uri, flags)
    , Query(uri, flags)
    , Frag(uri, flags)
{
}

::NUri::TUri &TPartNormalizers::GetMutableURI()
{
    IsSet_ = true;
    Scheme.Disable();
    Path.Disable();
    Query.Disable();
    Frag.Disable();
    return Uri_;
}

bool TPartNormalizers::Propagate()
{
    bool ok = false;
    if (Scheme.Propagate())
        ok = true;
    if (Host.Propagate())
        ok = true;
    if (Path.Propagate())
        ok = true;
    if (Query.Propagate())
        ok = true;
    if (Frag.Propagate())
        ok = true;
    if (Uri_.FldTryClr(::NUri::TUri::FieldUser))
        ok = true;
    if (Uri_.FldTryClr(::NUri::TUri::FieldPass))
        ok = true;
    return ok;
}

}
}
