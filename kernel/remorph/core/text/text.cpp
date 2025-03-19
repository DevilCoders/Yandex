#include "text.h"

#include <kernel/remorph/core/source_pos.h>
#include <util/string/split.h>

namespace NRemorph {

void Parse(TLiteralTableWtroka& lt, TVectorTokens& out_, const TUtf16String& s0) {
    const TUtf16String ds = u" \t";
    TVectorTokens out;
    TVector<TUtf16String> v;
    StringSplitter(s0).SplitBySet(ds.data()).SkipEmpty().Collect(&v);
    for (size_t i = 0; i < v.size(); ++i) {
        TTokenPtr t;
        const TUtf16String& s = v[i];
        if (s == u".") {
            t = new TTokenLiteral(TLiteral(0, TLiteral::Any));
        } else if (s == u"*") {
            t = new TTokenAsterisk();
        } else if (s == u"+") {
            t = new TTokenPlus();
        } else if (s == u"?") {
            t = new TTokenQuestion();
        } else if (s == u"|") {
            t = new TTokenPipe();
        } else if (s == u"^") {
            t = new TTokenLiteral(TLiteral(0, TLiteral::Bos));
        } else if (s == u"$") {
            t = new TTokenLiteral(TLiteral(0, TLiteral::Eos));
        } else if (s == u"(") {
            t = new TTokenLParen(TSourcePosPtr());
        } else if (s == u")") {
            t = new TTokenRParen(TSourcePosPtr());
        } else if (s == u"(?:") {
            t = new TTokenLParen(TSourcePosPtr(), false);
        } else if (s.StartsWith(u"(?<") && s.EndsWith(u">")) {
            size_t begin = s.find_first_of(u"<") + 1;
            size_t end = s.find_last_of(u">");
            t = new TTokenLParen(TSourcePosPtr(), WideToUTF8(s.substr(begin, end - begin)));
        } else if (s.StartsWith(u"{") && s.EndsWith(u"}")) {
            size_t begin = s.find_first_of(u"{") + 1;
            size_t end = s.find_last_of(u"}");
            size_t comma = s.find_first_of(u",");
            if (comma == TUtf16String::npos) {
                comma = end;
            }
            int from = -1;
            int to = -1;
            if (begin != comma) {
                from = ::FromString(WideToUTF8(s.substr(begin, comma - begin)));
            }
            if (comma == end) {
                to = from;
            } else if (comma + 1 != end) {
                to = ::FromString(WideToUTF8(s.substr(comma + 1, end - comma - 1)));
            }
            t = new TTokenRepeat(from, to);
        } else {
            t = new TTokenLiteral(lt.Add(s));
        }
        out.push_back(t);
    }
    out_.reserve(out_.size() + out.size());
    for (size_t i = 0; i < out.size(); ++i)
        out_.push_back(out[i]);
}

void Parse(const TUtf16String& s, TVectorSymbols& out) {
    static const TUtf16String ds = u" \t";
    StringSplitter(s).SplitBySet(ds.data()).SkipEmpty().Collect(&out);
}

void Parse(TVectorTokens& out, const TString& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        TTokenPtr t;
        switch (s[i]) {
                case '.':
                    t = new TTokenLiteral(TLiteral(0, TLiteral::Any));
                    break;
                case '*':
                    t = new TTokenAsterisk();
                    break;
                case '?':
                    t = new TTokenQuestion();
                    break;
                case '|':
                    t = new TTokenPipe();
                    break;
                case '(':
                    t = new TTokenLParen(new TSourcePos());
                    break;
                case ')':
                    t = new TTokenRParen(new TSourcePos());
                    break;
                case '+':
                    t = new TTokenPlus();
                    break;
                case '^':
                    t = new TTokenLiteral(TLiteral(0, TLiteral::Bos));
                    break;
                case '$':
                    t = new TTokenLiteral(TLiteral(0, TLiteral::Eos));
                    break;
                default:
                    t = new TTokenLiteral(TLiteral(s[i], TLiteral::Ordinal));
        }
        out.push_back(t);
    }
}

} // NRemorph
