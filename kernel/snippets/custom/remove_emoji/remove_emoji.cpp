#include "remove_emoji.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/titles/make_title/make_title.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

static const TString EMOJI_HOST_WHITELIST[] = {
    ".instagram.com",
};

static const THashSet<TUtf16String> EMOJI_QUERY_WHITELIST = {
    u"emoji",
    u"unicode",
    u"u",
    u"эмодзи",
    u"эмоджи",
    u"юникод",
    u"символ",
    u"смайл",
    u"смайлик",
};

static const wchar32 EMOJI_RANGES[][2] = {
    { 0x2600, 0x27BF }, // Misc symbols and Dingbats
    { 0xFE00, 0xFE0F }, // Variation Selectors
    { 0x2B00, 0x2BFF }, // Miscellaneous Symbols and Arrows
    { 0x1F600, 0x1F64F }, // Emoticons
    { 0x1F300, 0x1F5FF }, // Misc Symbols and Pictographs
    { 0x1F680, 0x1F6FF }, // Transport and Map
    { 0x1F900, 0x1F9FF }, // Supplemental Symbols and Pictographs
};

static const wchar16 ZERO_WIDTH_JOINER = 0x200D;

static bool IsWhitelistedHost(const TString& url) {
    const TString host = "." + TString{CutWWWPrefix(GetOnlyHost(url))};
    for (const auto& whiteHost : EMOJI_HOST_WHITELIST) {
        if (host.EndsWith(whiteHost)) {
            return true;
        }
    }
    return false;
}

static bool IsEmoji(wchar32 ch) {
    for (const wchar32* range : EMOJI_RANGES) {
        if (range[0] <= ch && ch <= range[1]) {
            return true;
        }
    }
    return false;
}

static bool IsWhitelistedQuery(const TQueryy& query) {
    for (const TQueryy::TPositionData& posData : query.Positions) {
        TUtf16String word = posData.OrigWord;
        word.to_lower();
        if (EMOJI_QUERY_WHITELIST.contains(word)) {
            return true;
        }
        for (const wchar16 *begin = word.data(), *end = begin + word.size(); begin < end; ) {
            if (IsEmoji(ReadSymbolAndAdvance(begin, end))) {
                return true;
            }
        }
    }
    return false;
}

size_t CountEmojis(const TUtf16String& str) {
    size_t cnt = 0;
    for (const wchar16 *begin = str.data(), *end = begin + str.size(); begin < end; ) {
        if (IsEmoji(ReadSymbolAndAdvance(begin, end))) {
            cnt++;
        }
    }
    return cnt;
}

static bool IsCollapsible(TUtf16String& str, size_t pos) {
    if (str[pos] == ZERO_WIDTH_JOINER) {
        return true;
    }
    if (!IsSpace(str[pos])) {
        return false;
    }
    return pos == 0 || pos + 1 == str.size() || IsSpace(str[pos + 1]) || IsPunct(str[pos + 1]);
}

bool RemoveEmojis(TUtf16String& str) {
    bool changed = false;
    size_t pos = 0;
    size_t charSize = 0;
    while (pos < str.size()) {
        charSize = W16SymbolSize(str.data() + pos, str.data() + str.size());
        if (IsEmoji(ReadSymbol(str.data() + pos, str.data() + str.size()))) {
            // string shrinks, pos stays
            str.remove(pos, charSize);
            changed = true;
            // remove extra spaces surrounding emoji
            while (pos > 0 && IsSpace(str[pos - 1])) {
                pos--;
            }
            while (pos < str.size() && IsCollapsible(str, pos)) {
                str.remove(pos, 1);
            }
        } else {
            pos += charSize;
        }
    }
    return changed;
}

bool NeedRemoveEmojis(const TConfig& cfg, const TQueryy& query, const TString& url) {
    if (cfg.ExpFlagOff("remove_emoji")) {
        return false;
    }
    if (IsWhitelistedHost(url)) {
        return false;
    }
    if (IsWhitelistedQuery(query)) {
        return false;
    }
    return true;
}

void TRemoveEmojiReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& repCtx = manager->GetContext();
    if (repCtx.Cfg.ExpFlagOff("remove_emoji")) {
        return;
    }
    if (IsWhitelistedHost(repCtx.Url)) {
        return;
    }
    if (IsWhitelistedQuery(repCtx.QueryCtx)) {
        return;
    }
    TReplaceResult result;
    bool snippetFound = false;
    bool changed = false;
    if (manager->IsReplaced()) {
        result = manager->GetResult();
        if (result.GetTextExt().Short) {
            snippetFound = true;
            if (CountEmojis(result.GetTextExt().Short) > repCtx.Cfg.AllowedSnippetEmojiCount()) {
                NSnippets::TMultiCutResult textExt = result.GetTextExt();
                changed |= RemoveEmojis(textExt.Short);
                if (textExt.Long) {
                    changed |= RemoveEmojis(textExt.Long);
                }
                result.UseText(textExt, result.GetTextSrc());
            }
        } else if (result.GetSnip()) {
            TUtf16String text = result.GetSnip()->GetRawTextWithEllipsis();
            if (text) {
                snippetFound = true;
                if (CountEmojis(text) > repCtx.Cfg.AllowedSnippetEmojiCount()) {
                    changed |= RemoveEmojis(text);
                    result.UseText(text, result.GetTextSrc());
                }
            }
        }
    }
    if (!snippetFound) {
        TUtf16String text = repCtx.Snip.GetRawTextWithEllipsis();
        if (text) {
            if (CountEmojis(text) > repCtx.Cfg.AllowedSnippetEmojiCount()) {
                changed |= RemoveEmojis(text);
            }
        }
        if (changed) {
            result.UseText(text, "remove_remoji");
        }
    }
    if (changed) {
        manager->Commit(result, MRK_REMOVE_EMOJI);
    }
}

}
