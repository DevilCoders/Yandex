#include "text_norm.h"

#include <util/charset/unidata.h>

namespace NOmni {

    TUtf16String NormalizeText(const TUtf16String& text) {
        TUtf16String normalizedText;
        bool lastWs = true;
        for (size_t i = 0; i < text.size(); ++i) {
            wchar16 w = text[i];
            if (IsAlnum(w)) {
                normalizedText.push_back(ToLower(w));
                lastWs = false;
            } else if (!lastWs) {
                normalizedText.push_back(' ');
                lastWs = true;
            }
        }
        if (normalizedText.size() && normalizedText.back() == ' ') {
            normalizedText.pop_back();
        }
        return normalizedText;
    }

} // NOmni
