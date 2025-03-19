#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include "flags.h"
#include "frag.h"
#include "host.h"
#include "path.h"
#include "qry.h"
#include "scheme.h"

namespace Nydx {
namespace NUriNorm {

class TPartNormalizers
{
    ::NUri::TUri &Uri_;
    const TFlags &Flags_;
    bool IsSet_;

public:
    TSchemeNormalizer Scheme;
    THostNormalizer Host;
    TPathNormalizer Path;
    TQueryNormalizer Query;
    TFragNormalizer Frag;
    TPartNormalizers(::NUri::TUri &uri
        , const TFlags &flags, const THostNormalizerBase &hostbase);
    bool AllDisabled() const
    {
        return Scheme.Disabled()
            && Path.Disabled() && Query.Disabled() && Frag.Disabled();
    }
    bool IsSet() const
    {
        return IsSet_;
    }
    const TFlags &Flags() const
    {
        return Flags_;
    }
    const ::NUri::TUri &GetURI() const
    {
        return Uri_;
    }
    ::NUri::TUri &GetMutableURI();
    void SetURI(const ::NUri::TUri &uri)
    {
        (GetMutableURI() = uri).Rewrite();
    }
    bool Propagate();
};

}
}
