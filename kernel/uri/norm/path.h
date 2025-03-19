#pragma once

/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include "base.h"
#include "coll.h"
#include "flags.h"

namespace Nydx {
namespace NUriNorm {

class TPathNormalizer
    : public TNormalizerBase<::NUri::TUri::FieldPath, TPathNormalizer>
{
    bool Parsed_;
    bool Changed_;
    const TFlags &Flags_;
    TStringBuf Path_;
    TString PathStr_;

public:
    TPathNormalizer(::NUri::TUri &uri, const TFlags &flags)
        : TdBase(uri, flags.GetNormalizePath())
        , Parsed_(false)
        , Changed_(false)
        , Flags_(flags)
    {}
    const TStringBuf &GetBuf()
    {
        Cleanup();
        return Path_;
    }
    TString Get() const {
        if (PathStr_.data() == Path_.data() && PathStr_.size() == Path_.size())
            return PathStr_; // allocation optimization
        else
            return TString(Path_);
    }
    void Trunc(size_t len)
    {
        Path_.Trunc(len);
        Parsed_ = true;
        Changed_ = true;
    }
    void SetParsed(const TString &path)
    {
        PathStr_ = path;
        SetParsedBuf(PathStr_);
    }
    void SetParsedBuf(const TStringBuf &path)
    {
        Parsed_ = true;
        SetImpl(path);
    }
    void Set(const TString &val)
    {
        PathStr_ = val;
        SetBuf(PathStr_);
    }
    void SetBuf(const TStringBuf &val)
    {
        CleanupImpl(val);
        Changed_ = true;
    }
    void UnSet()
    {
        Changed_ = false;
    }
    bool DoPropagate()
    {
        Cleanup();
        return Changed_ && SetFieldImpl(Path_);
    }

private:
    void SetImpl(const TStringBuf &path)
    {
        Changed_ = true;
        Path_ = path;
    }
    void Cleanup()
    {
        if (!Parsed_)
            SetBuf(GetField());
    }
    void CleanupImpl(const TStringBuf &val);
    void NormalizePath();
};

}
}
