#pragma once

#include <util/memory/tempbuf.h>
#include <library/cpp/charset/wide.h>
#include <util/string/cast.h>
#include <utility>
#include <library/cpp/wordpos/wordpos.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>

#include "decorators.h"

class TOutput {
private:
    IOutputStream& mStream;
private:
    TUtf16String PrepareText(const char* text, size_t len, ECharset charset) {
        TString buffer = ProcessControlCharacters(text, len);
        return CharToWide(buffer, charset);
    }

public:
    TOutput(IOutputStream& stream)
        : mStream(stream)
    {}

    void Write(const char* indent,
                     const char* name,
                     const TWordPosition& position,
                     bool overflow,
                     const char* attrname,
                     const wchar16* token = nullptr,
                     size_t tokenLength = 0)
    {
        mStream << indent << name << "\t"
            << "[" << position.Break() << "." << position.Word() << "]"
            << (overflow ? "!!" : "")
            << "\t";

        if (attrname && *attrname)
            mStream << "{" << attrname << "}:";
        if (token != nullptr) {
            mStream << "{";
            Decorate(token, tokenLength, mStream);
            mStream << "}";
        }

        mStream << Endl;
    }

    void Write(const char* indent,
                     const char* name,
                     const TWordPosition& position,
                     bool overflow,
                     const char* attrname,
                     const TWideToken& token)
    {
        if (token.SubTokens.size()) {
            TUtf16String s;
            for (size_t i = 0; i < token.SubTokens.size(); ++i) {
                const TCharSpan& span = token.SubTokens[i];
                if (i > 0) {
                    const TCharSpan& prev = token.SubTokens[i - 1];
                    const size_t prevEnd = prev.Pos + prev.Len + prev.SuffixLen;
                    if (prevEnd == (span.Pos - span.PrefixLen))
                        s.append(1, ' ');
                    else
                        s.append(token.Token + prevEnd, span.Pos - span.PrefixLen - prevEnd);
                }
                s.append(token.Token + span.Pos - span.PrefixLen, span.Len + span.PrefixLen + span.SuffixLen);
            }
            Write(indent, name, position, overflow, attrname, s.c_str(), s.size());
        } else
            Write(indent, name, position, overflow, attrname, token.Token, token.Leng);
    }
};
