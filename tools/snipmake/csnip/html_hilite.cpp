#include "html_hilite.h"

#include <util/charset/wide.h>

namespace NSnippets {

TUtf16String RehighlightAndHtmlEscape(const TUtf16String& s) {
    TUtf16String res;
    for (size_t i = 0; i < s.size(); ) {
        if (s[i] == 0x07 && i + 1 < s.size()) {
            if (s[i + 1] == '[' || s[i + 1] == ']') {
                res += UTF8ToWide(s[i + 1] == '[' ? "<b>" : "</b>");
                i += 2;
                continue;
            }
            if (s[i + 1] == '-' || s[i + 1] == '+') {
                res += UTF8ToWide(s[i + 1] == '+' ? "<l>" : "</l>");
                i += 2;
                continue;
            }
        }
        if (s[i] == '"' || s[i] == '\'') {
            res += UTF8ToWide(s[i] == '"' ? "&quot;" : "&apos;");
            ++i;
            continue;
        }
        if (s[i] == '<' || s[i] == '>') {
            res += UTF8ToWide(s[i] == '<' ? "&lt;" : "&gt;");
            ++i;
            continue;
        }
        if (s[i] == '&') {
            res += u"&amp;";
            ++i;
            continue;
        }
        res += s[i];
        ++i;
    }
    return res;
}

} //namespace NSnippets
