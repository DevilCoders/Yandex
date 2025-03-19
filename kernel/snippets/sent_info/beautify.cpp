#include "beautify.h"

#include <kernel/url_tools/url_tools.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/string/strip.h>

namespace NSnippets
{
    bool LooksLikeUrl(TWtringBuf s) {
        s = StripString(s);
        TIsUrlResult isUrl((IsUrl(WideToUTF8(s), 0)));
        return isUrl.IsUrl;
    }

    TWtringBuf CutFirstUrls(const TWtringBuf& sent)
    {
        size_t st = 0;
        bool wasCut = false;
        while (st < sent.size()) {
            size_t fn = sent.find(' ', st);
            if (fn == TUtf16String::npos) {
                fn = sent.size();
            }

            TIsUrlResult isUrl((IsUrl(WideToUTF8(TWtringBuf(sent.data() + st, sent.data() + fn)), 0)));
            if (!isUrl.IsUrl) {
                break;
            }
            wasCut = true;
            st = fn + 1;
        }
        if (!wasCut) {
            return sent;
        }

        for (size_t pos = st; pos < sent.size(); ++pos) {
            if (IsAlnum(sent[pos])) {
                return TWtringBuf(sent.data() + pos, sent.size() - pos);
            }
        }
        return TWtringBuf();
    }
}

