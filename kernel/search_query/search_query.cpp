#include <util/charset/utf8.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>

#include <util/memory/tempbuf.h>

#include "constants.h"
#include "search_query.h"



bool CheckAndFixQueryStringUTF8(const TStringBuf query, TString& dstQuery, const bool toLower, const bool isEmptyOk)
{
    const size_t length = query.length();

    // *2 is some rule of thumb to have some space if ToLower makes runes longer. This DO happen.
    TTempBuf dstStorage(length*2);
    ui8* dstBuf = (ui8*)dstStorage.Data();

    wchar32 rune;
    size_t  sRuneLen;

    const ui8* s    = (const ui8*)query.begin();
    const ui8* sEnd = (const ui8*)query.end();
    ui8* d    = dstBuf;
    ui8* dEnd = dstBuf + dstStorage.Size();

    bool wasSpace = false;

    for (; s != sEnd; s += sRuneLen) {
        if (d == dEnd)
            // not enough space in dstStorage
            return false;

        if (SafeReadUTF8Char(rune, sRuneLen, s, sEnd) != RECODE_OK)
            return false;

        if (toLower)
            rune = ToLower(rune);

        if (ui32(rune) <= 32 || ui32(rune) == 0x7f) { // 0x7f = UTF8_FIRST_CHAR
            wasSpace = true;
        } else {
            if (wasSpace) {
                *d++ = ' ';
                wasSpace = false;
            }
            size_t dRuneLen;
            if (SafeWriteUTF8Char(rune, dRuneLen, d, dEnd) != RECODE_OK)
                return false;
            d += dRuneLen; // it's safe, dstBuf won't overflow as it's checked in utf8_put_rune
        }
    }

    if (d == dstBuf) {
        if (!isEmptyOk)
            return false;
        dstQuery = TString();
        return true;
    }

    if (*dstBuf == ' ') // trim begin
        ++dstBuf;

    dstQuery = TString((char*)dstBuf, d - dstBuf);
    return true;
}


