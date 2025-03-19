#pragma once


#include <library/cpp/charset/wide.h>
#include <library/cpp/digest/old_crc/crc.h>
#include <library/cpp/numerator/numerate.h>

#include <kernel/indexer/face/inserter.h>

#include <yweb/antispam/common/shingle.h>

#include <util/string/ascii.h>
#include <util/string/printf.h>

extern const ui32 *crctab32;

inline bool IsSp(char c) {
    return !text_chars_yx[(unsigned char)c];
}

inline ui32 PrepareCRC() {
    return CRC32INIT ^ 0xFFFFFFFF;
}

inline void CalculateSentCRC(ui32& sentCrc, const char* sentBegin, const char* sentEnd, bool isNewSent) {
    if (isNewSent) { // различаем заголовки с разным числом предложений
        sentCrc = (sentCrc >> 8) ^ crctab32[(sentCrc ^ ui8('\n')) & 0xFF];
    }
    const char* c = sentBegin;
    while (c < sentEnd && IsSp(*c))
        c++;
    while (c < sentEnd) {
        if (IsSp(*c)) {
            while (c < sentEnd && IsSp(*c))
                c++;
            sentCrc = (sentCrc >> 8) ^ crctab32[(sentCrc ^ ui8(' ')) & 0xFF];
            continue;
        }
        sentCrc = (sentCrc >> 8) ^ crctab32[(sentCrc ^ ((ui8)*c)) & 0xFF];
        c++;
    }
}

inline void FinalizeCRC(ui32& sentCrc) {
    sentCrc = sentCrc ^ 0xFFFFFFFF;
}

class TTitleHandler : public INumeratorHandler
{
private:
    bool InsideTitle;
    TUtf16String CurrentSent;
    TVector<TUtf16String> Title;
    static constexpr size_t NormalizedTitleLengthLimit = 1024u;
public:
    TTitleHandler()
        : InsideTitle(false)
    {
    }

    TString GetTitle() const {
        TStringStream buffer;
        size_t buffersize = 0;
        for (size_t i = 0; i < Title.size(); ++i) {
            TString sent = WideToChar(Title[i], CODES_YANDEX);
            bool isprevspace = false;
            bool hasalnum = false;
            for (TString::const_iterator it = sent.begin(); it != sent.end(); ++it) {
                if (buffersize == NormalizedTitleLengthLimit) {
                    return buffer.Str();
                }

                if (CodePageByCharset(CODES_YANDEX)->IsAlnum(*it)) {
                    buffer << CodePageByCharset(CODES_YANDEX)->ToLower(*it);
                    ++buffersize;
                    isprevspace = false;
                    hasalnum = true;
                } else if (CodePageByCharset(CODES_YANDEX)->IsSpace(*it) && !isprevspace && hasalnum) {
                    buffer << ' ';
                    ++buffersize;
                    isprevspace = true;
                }
            }
        }

        return buffer.Str();
    }

    /**
     * @returns normalized title in the UTF-8 encoding.
     * Normalization is done as follows:
     *  * TNlpTokenizer produces tokens
     *  * Each token is treated as separate word
     *  * Max result length in utf16 code-points is NormalizedTitleLengthLimit
     *  * Division is guarntied to be made by token word
     */
    TString GetTokenNormalizedTitleUTF8() const;

    struct TPartTokenazationResult {
        bool IsBorderReached = false;
        size_t CodePointsResLength = 0;
        TString Result;
    };
    static TPartTokenazationResult GetTokenNormalizedTitleUTF8Impl(TWtringBuf text, size_t maxLen);

    /**
     * DEPRECATED (deletion is goin in SEARCH-8175), use GetTokenNormalizedTitleUTF8
     * @returns normalized title in the UTF-8 encoding.
     * NOTE: This function collides multitokens (like sanct-petersburg) into one continues word.
     *       Also it can join words from different sentences
     * Normalization is done as follows:
     *  * all non-alpanumeric characters are filtered out;
     *  * all spaces and several cosecutive ones are collapsed into one ordinary space;
     *  * title after previous modifications is truncated to @see NormalizedTitleLengthLimit number of symbols
     *    (not UTF-8 bytes).
     *  * eliminates trailing space.
     */
    TString GetNormalizedTitleUTF8() const {
        TUtf16String normalizedTitle;

        size_t titleSize = 0u;
        for (const TUtf16String& titlePart : Title) {
            titleSize += titlePart.size();
        }
        normalizedTitle.reserve(Min(titleSize, size_t(NormalizedTitleLengthLimit)));

        bool isPrevSpace = false;
        bool hasAlNum = false;
        for (const TUtf16String& titlePart : Title) {
            for (const auto& ch : titlePart) {
                if (normalizedTitle.size() == NormalizedTitleLengthLimit) {
                    break;
                }

                if (IsAlnum(ch)) {
                    normalizedTitle.push_back(ToLower(ch));
                    isPrevSpace = false;
                    hasAlNum = true;
                } else if (IsSpace(ch) && !isPrevSpace && hasAlNum) {
                    normalizedTitle.push_back(L' ');
                    isPrevSpace = true;
                }
            }
        }

        if (!normalizedTitle.empty() && IsSpace(normalizedTitle.back())) {
            normalizedTitle.pop_back();
        }

        return WideToUTF8(normalizedTitle);
    }

    TString GetTitleRawUTF8() const {
        TString ret;
        for (size_t i = 0; i < Title.size(); ++i) {
            ret += WideToUTF8(Title[i]);
        }
        return ret;
    }

    ui32 GetCRC() const {
        ui32 titleCrc = PrepareCRC();

        for (size_t i = 0; i < Title.size(); ++i) {
            TUtf16String title = Title[i];
            Strip(title);
            const TString sent = WideToChar(title, CODES_YANDEX);
            CalculateSentCRC(titleCrc, sent.data(), sent.data() + sent.size(), i > 0);
        }

        FinalizeCRC(titleCrc);
        return titleCrc;
    }

    void OnMoveInput(const THtmlChunk&, const TZoneEntry* zone, const TNumerStat&) override {
        if (zone && !zone->OnlyAttrs && zone->Name && strcmp("title", zone->Name) == 0) {
            InsideTitle = zone->IsOpen;
        }
    }

    void OnTokenStart(const TWideToken& tok, const TNumerStat&) override {
        if (InsideTitle) {
            CurrentSent.append(tok.Token, tok.Leng);
        }
    }

    void SkipSpaces(const wchar16*& tok, unsigned& len) {
        while ((len > 0) && (IsSpace(*tok))) {
            ++tok;
            --len;
        }
    }

    void OnSpaces(TBreakType type, const wchar16* tok, unsigned len, const TNumerStat&) override {
        if (InsideTitle) {
            if (CurrentSent.empty()) {
                SkipSpaces(tok, len);
            }
            if (len != 0) {
                CurrentSent.append(tok, len);

                if (IsSentBrk(type)) {
                    Title.push_back(CurrentSent);
                    CurrentSent.clear();
                }
            }
        }
    }

    void OnTextEnd(const IParsedDocProperties*, const TNumerStat&) override {
        Title.push_back(CurrentSent);
    }

    void InsertFactors(IDocumentDataInserter& inserter) const {
        inserter.StoreFullArchiveDocAttr("Title", GetTitle());
        inserter.StoreFullArchiveDocAttr("TitleCRC", Sprintf("%d", GetCRC()));
        inserter.StoreFullArchiveDocAttr("TitleRawUTF8", GetTitleRawUTF8());
        inserter.StoreFullArchiveDocAttr("TitleNormalizedUTF8", GetTokenNormalizedTitleUTF8());
    }
};
