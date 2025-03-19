#include "prepare_for_dssm.h"
#include <util/charset/unidata.h>
#include <util/charset/wide.h>

namespace NFacts {
    // Normalizes texts in the following sense:
    // 1) Removes stress marks, then
    // 2) replaces all non-alnum sequences with single spaces, then
    // 3) converts all letters to lowercase ones
    TString PrepareTextForDssm(const TUtf16String& wtext) {
        TUtf16String normalized;
        static const wchar16 STRESS_MARK = L'\u0301';

        bool nonAlnumSequence = false;
        for (auto it = wtext.begin(); it != wtext.end(); ++it) {
            // Note that this will work even if we encounter a surrogate pair as they are composed of
            // special codepoints (low/high surrogates) which are, in particular, non-alnums.
            const wchar16 wch = *it;

            if (IsAlnum(wch)) {
                nonAlnumSequence = false;
                normalized.append((wchar16) ToLower(wch));
            } else {
                if (wch != STRESS_MARK && !nonAlnumSequence) {
                    nonAlnumSequence = true;
                    normalized.append(' ');
                }
            }
        }

        return WideToUTF8(normalized);
    }
}
