#pragma once

/*
 *  Created on: Jan 27, 2012
 *      Author: albert@
 *
 * $Id$
 */


#include "base.h"
#include "flags.h"

namespace Nydx {
namespace NUriNorm {

class TFragNormalizer
    : public TNormalizerBase<::NUri::TUri::FieldFrag, TFragNormalizer>
{
    TStringBuf Val_; // reset by default
    TString Str_;
    bool Change_;

public:
    TFragNormalizer(::NUri::TUri &uri, const TFlags &flags);
    const TStringBuf &Get() const
    {
        return Val_;
    }
    bool IsSet() const
    {
        return Val_.data() == Str_.data();
    }
    const TStringBuf &GetSet() const
    {
        return Change_ && IsSet() ? Get() : GetField();
    }
    void Reset()
    {
        SetImpl();
    }
    void Keep()
    {
        Change_ = false;
    }
    void SetMem(const TStringBuf &buf)
    {
        SetImpl(buf);
    }
    void Set(const TString &str)
    {
        Str_ = str;
        SetImpl(Str_);
    }
    bool DoPropagate()
    {
        return Change_ && SetFieldImpl(Val_);
    }

private:
    void SetImpl(const TStringBuf &val = TStringBuf())
    {
        Change_ = true;
        Val_ = val;
    }
};

}
}
