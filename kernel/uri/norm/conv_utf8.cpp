/*
 *  Created on: Dec 7, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "conv_utf8.h"

#include <library/cpp/uri/uri.h>
#include <library/cpp/charset/recyr.hh>
#include <util/generic/maybe.h>
#include <util/charset/utf8.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace Nydx {
namespace NUriNorm {

static TStringBuf Convert(
    ECharset cset, const TStringBuf &src, char *dst, size_t dstlen)
{
    size_t nrd = 0;
    size_t nwr = 0;
    const RECODE_RESULT res = Recode(cset, CODES_UTF8
        , src.data(), dst, src.length(), dstlen, nrd, nwr);
    TStringBuf val;
    if (RECODE_OK == res)
        val = TStringBuf(dst, nwr);
    return val;
}

bool TConvUTF8::Normalize(
    ECharset cset, const TStringBuf &src, IOutputStream &dst)
{
    bool changed = false;
    do {
        // look for escaped chars
        if (TStringBuf::npos == src.find('%'))
            break;

        TTempBuf buf1(1 + src.length()); // space for null is required
        char *beg = buf1.Data();
        char *end = UrlUnescape(beg, src);
        const TStringBuf raw(beg, end);
        if (IsUtf(raw))
            break;

        const size_t need = 4 * raw.length();
        const size_t have = buf1.Size() - raw.length();

        TStringBuf rec;
        TMaybe<TTempBuf> buf2;
        if (have >= need) {
            rec = Convert(cset, raw, end, have);
        }
        else {
            buf2.ConstructInPlace(need);
            rec = Convert(cset, raw, buf2->Data(), buf2->Size());
        }
        if (!rec.IsInited())
            break;

        ::NUri::TUri::ReEncode(dst, rec);
        changed = true;
    }
    while (false);

    return changed;
}

}
}
