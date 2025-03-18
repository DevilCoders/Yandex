#include "page_template.h"

#include <library/cpp/html/pcdata/pcdata.h>

#include <util/generic/yexception.h>

#include <cctype>
#include <utility>


namespace NAntiRobot {


namespace {
    std::pair<bool, TStringBuf> ParseMaybeIdent(TStringBuf body, size_t& i) {
        ++i;

        Y_ENSURE(i < body.size(), "Bad template: unexpected EOF");

        bool isIdent = true;
        const auto start = i;

        while (body[i] != '%') {
            if (!std::isalnum(body[i]) && body[i] != '_') {
                isIdent = false;
                break;
            }

            ++i;

            Y_ENSURE(i < body.size(), "Bad template: unexpected EOF");
        }

        return {isIdent, body.substr(start, i - start)};
    }

    char HexDigit(char value) {
        if (value < 0xA) {
            return '0' + value;
        } else {
            return 'A' + value - 0xA;
        }
    }

    void EscapeJson(TStringBuf src, TString& dst) {
        for (char c : src) {
            switch (c) {
            case '\\':
                dst += "\\\\";
                break;
            case '"':
                dst += "\\\"";
                break;
            default:
                if (c < 0x20) {
                    dst.reserve(dst.size() + 4);
                    dst += "\\u";
                    dst.push_back(HexDigit(c >> 4));
                    dst.push_back(HexDigit(c & 0xF));
                } else {
                    dst += c;
                }

                break;
            }
        }
    }
}


TPageTemplate::TPageTemplate(TString body, TString resourceKey, EEscapeMode escapeMode)
    : ResourceKey(std::move(resourceKey))
    , Body(std::move(body))
    , EscapeMode(escapeMode)
{
    TStringBuf bodyRef(Body);
    size_t start = 0;

    for (size_t i = 0; i < bodyRef.size(); ++i) {
        if (bodyRef[i] != '%') {
            continue;
        }

        const auto j = i;
        const auto [isIdent, maybeIdent] = ParseMaybeIdent(bodyRef, i);

        if (!isIdent) {
            continue;
        }

        if (maybeIdent.empty()) {
            Chunks.push_back(TChunk{
                true,
                bodyRef.substr(start, j + 1 - start)
            });
        } else {
            if (start < j) {
                Chunks.push_back(TChunk{
                    true,
                    bodyRef.substr(start, j - start)
                });
            }

            Chunks.push_back(TChunk{false, maybeIdent});
        }

        start = i + 1;
    }

    if (start < bodyRef.size()) {
        Chunks.push_back(TChunk{true, bodyRef.substr(start)});
    }
}


TString TPageTemplate::Gen(
    const THashMap<TStringBuf, TStringBuf>& params
) const {
    TString ret;
    ret.reserve(Body.size());

    for (const auto& chunk : Chunks) {
        if (chunk.Verbatim) {
            ret += chunk.Value;
        } else if (const auto value = params.FindPtr(chunk.Value)) {
            switch (EscapeMode) {
            case EEscapeMode::Html:
                if (chunk.Value.StartsWith("NOESCAPE_")) {
                    ret += *value;
                } else {
                    ret += EncodeHtmlPcdata(*value);
                }
                break;
            case EEscapeMode::Json:
                EscapeJson(*value, ret);
                break;
            }
        }
    }

    return ret;
}


}
