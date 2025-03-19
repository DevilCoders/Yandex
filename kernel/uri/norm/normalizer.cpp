/*
 *  Created on: May 25, 2010
 *      Author: albert@
 *
 * $Id$
 */

#include <library/cpp/charset/recyr.hh>

#include <util/memory/tempbuf.h>

#include <library/cpp/html/entity/htmlentity.h>

#include "normalizer.h"
#include "host.h"
#include "host_spec.h"
#include "path.h"
#include "part.h"
#include "qry.h"

namespace Nydx {
namespace NUriNorm {

/**
 * Normalizes the URL.
 * -  If no scheme ("XXX:/") is found, prepend "http://".
 * -  Optionally convert all pluses to escaped spaces (%20); normally,
 *    only the query part will (and should) undergo that.
 * -  Optionally lowercase path (if --path:low) or query (--qry:low) but not
 *    if --uri:low was specified (then the whole URI is already lowercased)
 * -  Normalize the host portion of the parsed URL
 *    (@see THostNormalizer::NormalizeHost).
 * -  Fix the path:
 *    a. remove the last part of the URL if it is one of the typical default
 *       files (@see DefaultPathSuffix);
 *    a. if --path:rmslash is used and path ends in a '/', truncate it
 *       (destructive but necessary to match URLs from various sources);
 *    c. if path has become empty, make it a single '/'.
 * -  Normalize the query part (@see CleanupQuery).
 * @param[in] src the string containing the URL.
 * @param[out] dst the @c ::NUri::TUri instance to get the fixed and parsed URL.
 * @return true if the URL has changed.
 */
bool TUriNormalizer::NormalizeUriImpl(::NUri::TUri &dst) const
{
    TPartNormalizers norm(dst, *this, HostBase_);

    // normalize scheme
    norm.Scheme.MakeHttpIfEmpty();

    do {
        if (!GetNormalizeHostSpecific())
            break;
        if (norm.AllDisabled())
            break;
        NUriNorm::THostSpecific::Normalize(norm);
        if (!norm.IsSet())
            break;
        return NormalizeUriImpl(dst);
    }
    while (false);

    norm.Propagate();

    // if we changed anything in memory only,
    // rewrite as that memory is local to this method
    dst.Rewrite();

    return true;
}

bool TUriNormalizer::ParseImpl(TStringBuf src, ::NUri::TUri &dst) const
{
    TTempBuf buf(src.length() * 4);
    char *out = buf.Data();
    size_t outsize = buf.Size();

    if (GetDecodeHTML()) {
        const TStringBuf decoded = HtTryEntDecodeAsciiCompat(
            src, out, outsize, GetCharsetURI(), CODES_UTF8);
        if (decoded.data() != nullptr)
            src = decoded;
    }
    else if (CODES_UTF8 != GetCharsetURI()) {
        size_t nrd, nwr;
        if (RECODE_OK == Recode(GetCharsetURI(), CODES_UTF8
            , src.data(), out, src.length(), outsize, nrd, nwr))
            src = TStringBuf(out, nwr);
    }

    const bool ok = ::NUri::TUri::ParsedOK == Parse(dst, src);
    if (!ok)
        dst.Clear();

    return ok;
}

}
}
