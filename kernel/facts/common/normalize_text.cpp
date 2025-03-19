#include "normalize_text.h"

namespace NUnstructuredFeatures {

    TUtf16String NormalizeText(const TWtringBuf& text) {
        static const TUtf16String::value_type STRESS_MARK = L'\u0301';

        TUtf16String normalizedText;
        normalizedText.reserve(text.size());

        bool putSpace = false;
        for (const TUtf16String::value_type ch : text) {
            if (IsAlnum(ch)) {
                if (putSpace) {
                    normalizedText.append(' ');
                    putSpace = false;
                }
                normalizedText.append(static_cast<TUtf16String::value_type>(ToLower(ch)));
            } else {
                putSpace = !normalizedText.empty() && ch != STRESS_MARK;
            }
        }

        return normalizedText;
    }

    TString NormalizeTextUTF8(const TString& text) {
        return WideToUTF8(NormalizeText(UTF8ToWide(text)));
    }
}
