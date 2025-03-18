#pragma once
#include <util/generic/strbuf.h>
#include <util/charset/wide.h>

TUtf16String NoHtml(const char* text, size_t len);
inline TString NoHtml(const TStringBuf& text) {
    return WideToUTF8(NoHtml(text.data(), text.length()));
}
inline TUtf16String NoHtml(const TWtringBuf& text) {
    TString utf8 = WideToUTF8(text);
    return NoHtml(utf8.data(), utf8.size());
}

TUtf16String HtmlEscape(const char* text, size_t len);
inline TString HtmlEscape(const TStringBuf& utf8) {
    return WideToUTF8(HtmlEscape(utf8.data(), utf8.size()));
}
inline TUtf16String HtmlEscape(const TWtringBuf& text) {
    TString utf8 = WideToUTF8(text);
    return HtmlEscape(utf8.data(), utf8.size());
}
