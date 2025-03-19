#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include <library/cpp/uri/uri.h>

namespace Nydx {
namespace NUriNorm {

template <::NUri::TUri::EField TpField, typename TpDeriv> class TNormalizerBase
{
public:
    typedef TNormalizerBase TdBase;

protected:
    ::NUri::TUri &Uri_;
    bool Disabled_;

public:
    TNormalizerBase(::NUri::TUri &uri, bool normalize = true)
        : Uri_(uri)
        , Disabled_(!normalize)
    {}
    bool Disabled() const
    {
        return Disabled_;
    }
    void Disable()
    {
        Disabled_ = true;
    }
    void Enable()
    {
        Disabled_ = false;
    }
    const ::NUri::TUri &GetURI() const
    {
        return Uri_;
    }
    const TStringBuf &GetField(::NUri::TUri::EField field = TpField) const
    {
        return GetURI().GetField(field);
    }
    bool Propagate()
    {
        return !Disabled() && Self()->DoPropagate();
    }

protected:
    ::NUri::TUri &Uri()
    {
        return Uri_;
    }
    template <typename T> bool SetField(const T &val)
    {
        return !Disabled() && SetFieldImpl(val);
    }
    template <typename T> bool SetFieldImpl(const T &val)
    {
        return Uri().FldMemSet(TpField, val);
    }
    TpDeriv *Self()
    {
        return static_cast<TpDeriv *>(this);
    }
};

}
}
