#include <kernel/search_types/search_types.h>
#include "dt_gztinput.h"

#include <util/charset/wide.h>

namespace NGzt {

const wchar16 APOS[] = {'\'', 0x60, 0x2019};
const TWtringBuf APOS_BUF{APOS, Y_ARRAY_SIZE(APOS)};

std::pair<size_t, size_t> TDTGztInput::CalcNormalizedText(const TDirectTextEntry2& entry) {
    if (entry.Token == nullptr) {
        return std::make_pair(0, 0);
    }

    const size_t from = TextBuffer.size();
    if (0 != entry.LemmatizedTokenCount) {
        size_t subTokenCount = entry.LemmatizedToken[entry.LemmatizedTokenCount - 1].FormOffset + 1;

        for (ui32 i = 0; i < entry.LemmatizedTokenCount; ++i) {
            const TLemmatizedToken& token = entry.LemmatizedToken[i];

            // Detect the full lemma. It starts from the zero position and has no joins from the right
            if (1 == subTokenCount
                || (0 == token.FormOffset
                    && (0 == (FORM_HAS_JOINS & token.Flags) || 0 == (FORM_RIGHT_JOIN & token.Joins)))) {
                wchar16 buffer[MAXKEY_BUF];
                const size_t len = NIndexerCore::ConvertKeyText(token.FormaText, buffer);
                TextBuffer.append(buffer, len);
                return std::make_pair(from, len);
            }
        }
    }
    const size_t len = entry.Token.size();
    TextBuffer.ReserveAndResize(from + len);
    ::ToLower(entry.Token.data(), len, TextBuffer.begin() + from);
    return std::make_pair(from, len);
}

// For Turkish language truncate suffix after apostrophe and use it as extra form
std::pair<size_t, size_t> TDTGztInput::CalcExtraForm(const TDirectTextEntry2& entry) {
    if (entry.Token == nullptr) {
        return std::make_pair(0, 0);
    }

    const auto pos = entry.Token.find_first_of(APOS_BUF);
    if (pos == TWtringBuf::npos) {
        return std::make_pair(0, 0);
    }
    const size_t from = TextBuffer.size();
    TextBuffer.ReserveAndResize(from + pos);
    ::ToLower(entry.Token.data(), pos, TextBuffer.begin() + from);
    return std::make_pair(from, pos);
}

void TDTGztInput::CalcTexts(const TLangMask& langs) {
    NormPos.reserve(Count);
    for (size_t i = 0; i < Count; ++i) {
        NormPos.push_back(CalcNormalizedText(Entries[i]));
    }
    if (langs.Test(LANG_TUR)) {
        ExtraPos.reserve(Count);
        for (size_t i = 0; i < Count; ++i) {
            ExtraPos.push_back(CalcExtraForm(Entries[i]));
        }
    }
}

} // NGzt

