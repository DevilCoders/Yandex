#pragma once

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/buffer.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

static TString Entity(wchar32 ch) {
    return "&#" + IntToString<10>(ch) + ";";
}

TString ProcessControlCharacters(const char* text, size_t length) {
    TString res;

    const char* first = text;
    const char* const last = text + length;
    for (; first != last; ++first) {
        char symbol = *first;
        size_t index = static_cast<unsigned char>(symbol);

        if (index < 32) {
            res += Entity(index);
        } else {
            res += symbol;
        }
    }

    return res;
}

//! @param text     utf8 encoded text
void Decorate(const char* text, size_t length, IOutputStream& output) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
    const unsigned char* const e = p + length;
    while (p != e) {
        wchar32 c;
        const unsigned char* const s = p;
        if (ReadUTF8CharAndAdvance(c, p, e) != RECODE_OK)
            break;

        if (c < 32) {
            output << Entity(c);
        } else {
            output.Write(s, p - s);
        }
    }
}

void Decorate(const wchar16* text, size_t length, IOutputStream& output) {
    const TString s = WideToUTF8(text, length);
    Decorate(s.c_str(), s.size(), output);
}
