#include "canonize_unicode.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <util/charset/wide.h>
#include <library/cpp/unicode/normalization/normalization.h>

namespace NSnippets {

static const wchar32 DIACRITICAL_MARKS_RANGES[][2] = {
    { 0x0300, 0x036F }, // Combining Diacritical Marks
    { 0x1AB0, 0x1AFF }, // Combining Diacritical Marks Extended
    { 0x1DC0, 0x1DFF }, // Combining Diacritical Marks Supplement
    { 0x20D0, 0x20FF }, // Combining Diacritical Marks for Symbols
    { 0xFE20, 0xFE2F }, // Combining Half Marks

};

static bool IsDiacriticalMark(wchar32 ch) {
    for (const wchar32* range : DIACRITICAL_MARKS_RANGES) {
        if (range[0] <= ch && ch <= range[1]) {
            return true;
        }
    }
    return false;
}

bool CanonizeUnicode(TUtf16String& str) {
    TUtf16String result;
    result.reserve(str.size());
    const TUtf32String normalized = Normalize<NUnicode::ENormalization::NFC>(TUtf32String::FromUtf16(str));
    for (size_t pos = 0; pos < normalized.size(); ++pos) {
        if (!IsDiacriticalMark(normalized[pos])) {
            result += UTF32ToWide(&normalized[pos], 1);
        }
    }

    if (str != result) {
        str = result;
        return true;
    }
    return false;
}


void TCanonizeUnicodeReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& repCtx = manager->GetContext();
    if (!repCtx.Cfg.ShouldCanonizeUnicode()) {
        return;
    }
    TReplaceResult result;
    bool snippetFound = false;
    bool changed = false;
    if (manager->IsReplaced()) {
        result = manager->GetResult();
        if (result.GetTextExt().Short) {
            snippetFound = true;
            NSnippets::TMultiCutResult textExt = result.GetTextExt();
            changed |= CanonizeUnicode(textExt.Short);
            if (textExt.Long) {
                changed |= CanonizeUnicode(textExt.Long);
            }
            result.UseText(textExt, result.GetTextSrc());
        } else if (result.GetSnip()) {
            TUtf16String text = result.GetSnip()->GetRawTextWithEllipsis();
            if (text) {
                snippetFound = true;
                changed |= CanonizeUnicode(text);
                result.UseText(text, result.GetTextSrc());
            }
        }
    }
    if (!snippetFound) {
        TUtf16String text = repCtx.Snip.GetRawTextWithEllipsis();
        if (text) {
            changed |= CanonizeUnicode(text);
        }
        if (changed) {
            result.UseText(text, "canonize_unicode");
        }
    }
    if (changed) {
        manager->Commit(result, MRK_REMOVE_EMOJI);
    }
}

}
