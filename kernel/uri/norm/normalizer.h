#pragma once

/*
 *  Created on: May 25, 2010
 *      Author: albert@
 *
 * $Id$
 */


#include <library/cpp/uri/uri.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/noncopyable.h>

#include <kernel/mirrors/mirrors.h>
#include <kernel/mirrors/mirrors_trie.h>

#include "flags.h"
#include "host.h"

namespace Nydx {
namespace NUriNorm {

class TUriNormalizer
    : public TNonCopyable
    , public TFlags
{
private:
    THostNormalizerBase HostBase_;

public:
    TUriNormalizer()
        : TFlags()
    {}

protected:
    bool NormalizeHostImpl(TStringBuf &host) const
    {
        return HostBase_.NormalizeHost(host, *this);
    }

    bool NormalizeUriImpl(::NUri::TUri &dst) const;

    bool ParseImpl(TStringBuf src, ::NUri::TUri &dst) const;

public:
    /**
     * Normalizes the hostname unless it was disabled (e.g., --host:fix=0).
     * 1. Lowercases the hostname.
     * 2. If mirror database is used, replaces with the main mirror if found.
     * 3. Optionally removes the 'www.' prefix (--host:rmwww).
     * @param[in,out] host the hostname to normalize
     * @return true if the hostname has changed
     */
    bool NormalizeHost(const TStringBuf &src, TString &dst) const
    {
        dst.assign(src.data(), src.size());
        return GetNormalizeHost() ? DoNormalizeHost(dst) : true;
    }
    bool DoNormalizeHost(TString &host) const
    {
        const bool changed = host.to_lower();
        TStringBuf buf = host;
        if (!NormalizeHostImpl(buf))
            return changed;
        host = buf;
        return true;
    }

public:
    bool DoNormalizeUrl(const TStringBuf &src, ::NUri::TUri &dst) const
    {
        return ParseImpl(src, dst) && NormalizeUriImpl(dst);
    }

    bool TryNormalizeUrl(const TStringBuf &src, ::NUri::TUri &dst) const
    {
        return ParseImpl(src, dst) && (!GetNormalizeURLs() || NormalizeUriImpl(dst));
    }

    bool DoNormalizeUrl(IOutputStream &out, const TStringBuf &src) const
    {
        ::NUri::TUri dst;
        if (!DoNormalizeUrl(src, dst))
            return false;
        dst.Print(out, PrintFlags());
        return true;
    }

    int PrintFlags() const
    {
        int flags = ::NUri::TUri::FlagAll;
        if (GetRemoveScheme())
            flags &= ~::NUri::TUri::FlagScheme;
        return flags;
    }

public:
    // src may not point to buf
    bool NormalizeUrl(const TStringBuf &src, TString &buf) const
    {
        if (GetNormalizeURLs()) {
            TStringOutput out(buf);
            if (DoNormalizeUrl(out, src))
                return true;
        }
        buf.AssignNoAlias(src);
        return false;
    }
    TString NormalizeUrl(const TStringBuf &src) const
    {
        TString buf;
        NormalizeUrl(src, buf);
        return buf;
    }

public:
    void SetMirrorFile(const char *filename)
    {
        EnableNormalizeHost();
        HostBase_.SetMirrorFile(filename);
    }
};

}

typedef NUriNorm::TUriNormalizer TUriNormalizer;

}
