#pragma once

/*
 *  Created on: May 25, 2010
 *      Author: albert@
 *
 * $Id$
 */


#include "base.h"
#include "flags.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

#include <kernel/mirrors/mirrors.h>
#include <kernel/mirrors/mirrors_trie.h>

namespace Nydx {
namespace NUriNorm {

class THostNormalizerBase
{
private:
    THolder<mirrors_mapped> MirMap_;
    THolder<TMirrorsMappedTrie> MirTrie_;
    mutable TString MirHost_;

public:
    bool SetMirrorFile(const char *mirfile);

private:
    void UnmirrorHostHash(const TStringBuf &host, TStringBuf &res) const
    {
        res = MirMap_->check(host);
        if (!res.empty() || 0 == res.compare(host))
            res.Clear();
    }
    void UnmirrorHostTrie(const TStringBuf &host, TStringBuf &res) const
    {
        const TMirrorsMappedTrie::THostId hostId = MirTrie_->StringToId(host);
        if (0 == hostId)
            return;
        const TMirrorsMappedTrie::THostId mainId = MirTrie_->GetMain(hostId);
        if (mainId == hostId)
            return;
        res = MirTrie_->IdToString(mainId, MirHost_);
    }

public:
    TStringBuf UnmirrorHost(const TStringBuf &host) const
    {
        TStringBuf res;
        if (nullptr != MirTrie_.Get())
            UnmirrorHostTrie(host, res);
        else if (nullptr != MirMap_.Get())
            UnmirrorHostHash(host, res);
        return res;
    }

    /**
     * Normalizes the hostname (must already be lowercased).
     * - If mirror database is used, replaces with the main mirror if found.
     * - Optionally removes the 'www.' prefix (--removewww).
     * @param[in,out] host the hostname to normalize
     * @return true if the hostname has changed
     */
    bool NormalizeHost(TStringBuf &host, const TFlags &flags) const;
};

class THostNormalizer
    : public TNormalizerBase<::NUri::TUri::FieldHost, THostNormalizer>
{
    bool Parsed_;
    bool Changed_;
    const TFlags &Flags_;
    const THostNormalizerBase &Base_;
    TStringBuf Buf_;
    TString Str_;

public:
    THostNormalizer(::NUri::TUri &uri, const TFlags &flags, const THostNormalizerBase &base)
        : TdBase(uri, flags.GetNormalizeHost())
        , Parsed_(false)
        , Changed_(false)
        , Flags_(flags)
        , Base_(base)
    {}
    const TStringBuf &Get()
    {
        Cleanup();
        return Buf_;
    }
    void SetParsed(const TString &str)
    {
        Str_ = str;
        SetImpl();
        Parsed_ = true;
    }
    void Set(const TStringBuf &val)
    {
        CleanupImpl(val);
        Changed_ = true;
    }
    bool DoPropagate()
    {
        Cleanup();
        return Changed_ && SetFieldImpl(Buf_);
    }

private:
    void SetImpl()
    {
        Changed_ = true;
        Buf_ = Str_;
    }
    void Cleanup()
    {
        if (!Parsed_) {
            Parsed_ = true;
            CleanupImpl(GetField());
        }
    }

    void CleanupImpl(const TStringBuf &val)
    {
        Buf_ = val;
        if (Base_.NormalizeHost(Buf_, Flags_))
            Changed_ = true;
    }
};


}
}
