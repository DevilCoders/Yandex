/*
 *  Created on: Dec 7, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "coll.h"
#include "conv_utf8.h"
#include "host_wiki.h"

#include <util/string/subst.h>

namespace Nydx {
namespace NUriNorm {
namespace NWiki {

struct TCharsetMap
    : public TMapT<TStringBuf, ECharset>
{
    TCharsetMap()
    {
        Val("ru") = CODES_WIN; // cp-1251
        Val("he") = CODES_WINDOWS_1255;
        Val("tr") = CODES_WINDOWS_1254;
        Val("lt") = CODES_WINDOWS_1257;
        Val("lv") = CODES_WINDOWS_1257;
        Val("ee") = CODES_WINDOWS_1257;
    }
};

static const TCharsetMap CharsetMap;

/**
 * Normalizes the Wikipedia path.
 */
class THostSpecImpl
{
public:
    static bool Normalize(const TStringBuf &hpref, TPartNormalizers &norm);

private:
    static bool NormalizePath(const TStringBuf &hpref, TPartNormalizers &norm);
    static void NormalizeWindex(TPartNormalizers &norm);
};

static const char* FindFirstNotUnderscore(const char* first, const char* last) {
    for (; first != last; ++first)
        if (*first != '_')
            return first;
    return last;
}

static const char* FindLastNotUnderscore(const char* first, const char* last) {
    --last;
    --first;
    for (; first != last; --last)
        if (*last != '_')
            return last;
    return nullptr;
}

/// Removes repeated underscores and all underscores at start and at end of
/// query and namespace
/// Returns removed underscores count
static void RemoveRepeatedUnderscoresImpl(char*& dst, const char*& src, const char* end) {
    src = FindFirstNotUnderscore(src, end);
    if (src == end)
        return;

    end = FindLastNotUnderscore(src, end);
    if (!end)
        return;
    ++end;
    Y_ASSERT(src <= end);

    while (src != end) {
        *dst++ = *src;
        if (*src == '_') { // repeated?
            src = FindFirstNotUnderscore(src, end); // skip (if any underscores are repeated)
            Y_ASSERT(src != end);
        } else
            ++src;
    }
}

static size_t RemoveRepeatedUnderscores(TString& path, size_t startPos) {
    char* dst = path.begin() + startPos; // begin() clones string if it is shared
    const char* src = path.data() + startPos; // at start dst and src point to the same address
    Y_ASSERT(path.IsDetached());
    Y_ASSERT(src == dst);
    const char* end = path.data() + path.size();

    // namespace part
    const char* found = Find(src, end, ':');
    if (found != end) {
        RemoveRepeatedUnderscoresImpl(dst, src, found); // remove underscores in namespace
        *dst++ = ':';
        src = found + 1;
    }

    // query part
    RemoveRepeatedUnderscoresImpl(dst, src, end); // remove underscores in query

    const size_t newSize = dst - path.data();
    const size_t result = path.size() - newSize;
    path.ReserveAndResize(newSize); // sets new size of string // in our case it doesn't reserve any memory
    Y_ASSERT(path.size() == newSize);
    return result;
}

bool THostSpecImpl::Normalize(const TStringBuf &hpref, TPartNormalizers &norm)
{
    bool changed = false;

    if (!norm.Scheme.Disabled())
        changed = norm.Scheme.MakeHttp();

    bool normpath = !norm.Path.Disabled();

    if (normpath && norm.Path.GetBuf() == "/") {
        TQueryParams &cgi = norm.Query.GetCGI();
        TQueryParams::TIter itb, ite;
        cgi.Get("title", itb, ite);
        if (itb != ite) {
            norm.Path.SetParsed(TString::Join("/wiki/", itb->Val()));
            norm.Query.ResetCGI();
            normpath = false;
        }
        changed = true;
    }

    static const TString windex = "/w/index.php";

    do {
        if (norm.Path.GetField() == windex) {
            // index.php was probably removed
            norm.Path.UnSet();
            NormalizeWindex(norm);
            normpath = false;
            break;
        }

        if (norm.Path.GetBuf().StartsWith("/wiki/")) {
            const TStringBuf tail = norm.Path.GetBuf().SubStr(6);
            if (normpath && "Search" == tail) {
                norm.Path.SetParsed(windex);
                NormalizeWindex(norm);
                changed = true;
                normpath = false;
            }
            else if (!norm.Query.Disabled()) {
                norm.Query.ResetCGI();
                changed = true;
            }
            break;
        }
    }
    while (false);

    if (normpath && NormalizePath(hpref, norm))
        changed = true;

    TString path = norm.Path.Get();
    size_t lastSlash = path.find_last_of('/');
    bool replaced = false;
    if (lastSlash != TString::npos && lastSlash < path.size() - 1) {
        const size_t startPos = lastSlash + 1;

        // http://en.wikipedia.org/wiki/Help:URL#URLs_of_Wikipedia_pages
        // http://en.wikipedia.org/wiki/Wikipedia:Canonical#Conversion_to_canonical_form
        // If constructing URLs for Wikipedia pages, remember to convert spaces into underscores
        if (SubstGlobal(path, "%20", "_", startPos) != 0)
            replaced = true;

        // Replace repeated underscores with one
        // Delete underscores at start and at end of query or namespace
        if (RemoveRepeatedUnderscores(path, startPos) != 0)
            replaced = true;

        if (replaced)
            norm.Path.SetParsed(path);
    }

    return changed || replaced;
}

void THostSpecImpl::NormalizeWindex(TPartNormalizers &norm)
{
    if (!norm.Query.Disabled()) {
        TQueryParams &cgi = norm.Query.GetCGI();
        cgi.EraseAll("title");
        cgi.EraseAll("fulltext");
    }
}


bool THostSpecImpl::NormalizePath(const TStringBuf &hpref, TPartNormalizers &norm)
{
    const ECharset *cset = CharsetMap.Get(hpref);
    if (nullptr == cset)
        return false;

    TString escpath;
    TStringOutput out(escpath);
    if (!TConvUTF8::Normalize(*cset, norm.Path.GetBuf(), out))
        return false;

    norm.Path.SetParsed(escpath);
    return true;
}

bool THostSpec::Normalize(const TStringBuf &hpref, TPartNormalizers &norm) const
{
    return THostSpecImpl::Normalize(hpref, norm);
}

}
}
}
