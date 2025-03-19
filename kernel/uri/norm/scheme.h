#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include "base.h"
#include "flags.h"

namespace Nydx {
namespace NUriNorm {

class TSchemeNormalizer
    : public TNormalizerBase<::NUri::TUri::FieldScheme, TSchemeNormalizer>
{
    TStringBuf Scheme_;

public:
    TSchemeNormalizer(::NUri::TUri &uri, const TFlags &flags)
        : TdBase(uri, true)
    {
        Y_UNUSED(flags);
    }
    void Set(const TStringBuf &val)
    {
        Scheme_ = val;
    }
    bool DoPropagate()
    {
        return Scheme_.IsInited() && SetFieldImpl(Scheme_);
    }
    bool MakeHttp()
    {
        return ::NUri::TUri::SchemeHTTP != GetURI().GetScheme() && MakeHttpImpl();
    }
    bool MakeHttpIfEmpty()
    {
        return ::NUri::TUri::SchemeEmpty == GetURI().GetScheme() && MakeHttpImpl();
    }

private:
    bool MakeHttpImpl()
    {
        return !GetField(::NUri::TUri::FieldHost).empty() && SetField("http");
    }
};

}
}
