#include "out.h"
#include "def.h"

#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/stream/str.h>

#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/spec/lextype.h>

namespace {
    struct TNameMap: public TMap<TIrregTag, const char*> {
        inline TNameMap() {
            (*this)[IRREG_none] = "IRREG_none";
            (*this)[IRREG_B] = "IRREG_B";
            (*this)[IRREG_BIG] = "IRREG_BIG";
            (*this)[IRREG_FONT] = "IRREG_FONT";
            (*this)[IRREG_I] = "IRREG_I";
            (*this)[IRREG_S] = "IRREG_S";
            (*this)[IRREG_SMALL] = "IRREG_SMALL";
            (*this)[IRREG_STRIKE] = "IRREG_STRIKE";
            (*this)[IRREG_TT] = "IRREG_TT";
            (*this)[IRREG_U] = "IRREG_U";
            (*this)[IRREG_SUB] = "IRREG_SUB";
            (*this)[IRREG_SUP] = "IRREG_SUP";
            (*this)[IRREG_DEL] = "IRREG_DEL";
            (*this)[IRREG_INS] = "IRREG_INS";
            (*this)[IRREG_A] = "IRREG_A";
            (*this)[IRREG_BDO] = "IRREG_BDO";
            (*this)[IRREG_EM] = "IRREG_EM";
            (*this)[IRREG_STRONG] = "IRREG_STRONG";
            (*this)[IRREG_NOINDEX] = "IRREG_NOINDEX";
            (*this)[IRREG_NOSCRIPT] = "IRREG_NOSCRIPT";
            (*this)[IRREG_HLWORD] = "IRREG_HLWORD";
            // (*this)[IRREG_scope] = "IRREG_scope";
        }

        static inline const TNameMap& Instance() {
            return *Singleton<TNameMap>();
        }
    };
}

template <>
void Out<TIrregTag>(IOutputStream& os, TTypeTraits<TIrregTag>::TFuncParam n) {
    const TNameMap& nameMap = TNameMap::Instance();
    bool is_first = true;

    for (const auto& i : nameMap) {
        if (n & i.first) {
            if (!is_first) {
                os << " | ";
                is_first = false;
            }
            os << i.second;
        }
    }
}

template <>
void Out<NHtmlLexer::TToken>(IOutputStream& os, TTypeTraits<NHtmlLexer::TToken>::TFuncParam t) {
    HTLEX_TYPE type = t.Type;

    os << type << '\t';
    if (type == HTLEX_START_TAG || type == HTLEX_END_TAG)
        os << t.HTag->lowerName << '\t';
    else if (type == HTLEX_TEXT && t.IsWhitespace)
        os << " (sp) ";

    if (t.Text && t.Leng)
        // os.write(t.Text, t.Leng);
        os << nicer(TString(t.Text, t.Leng));
}

class TAttrsStreamer2 {
private:
    const char* Token;
    const NHtml::TAttribute* Attrs;
    unsigned Cnt;

public:
    TAttrsStreamer2(const char* token, const NHtml::TAttribute* attrs, unsigned cnt)
        : Token(token)
        , Attrs(attrs)
        , Cnt(cnt)
    {
    }
    friend IOutputStream& operator<<(IOutputStream& os, const TAttrsStreamer2& s);
};

IOutputStream& operator<<(IOutputStream& os, const TAttrsStreamer2& s) {
    for (unsigned i = 0; i < s.Cnt; ++i) {
        /*
        os << ' '
            << s.attrs[i].Name.Start << ',' << s.attrs[i].Name.Leng
            << '='
            << s.attrs[i].Value.Start << ',' << s.attrs[i].Value.Leng;
        os << ' ';
    */

        os.Write((const char*)(s.Token + s.Attrs[i].Name.Start), s.Attrs[i].Name.Leng);
        os << "=";
        os.Write((const char*)s.Token + s.Attrs[i].Value.Start, s.Attrs[i].Value.Leng);
        os << " ";
    }
    return os;
}

void ShowAttrs(IOutputStream& os, const char* token, const NHtml::TAttribute* attrs, unsigned n) {
    os << TAttrsStreamer2(token, attrs, n);
}

namespace {
    struct TRep: public TMap<char, TString> {
        inline TRep() {
            (*this)['\0'] = "\\0";
            (*this)['\t'] = "\\t";
            (*this)['\r'] = "\\r";
            (*this)['\n'] = "\\n";
            (*this)['\f'] = "\\f";
            (*this)['\v'] = "\\v";
            (*this)['\\'] = "\\\\";
            (*this)['\''] = "\\'";
            (*this)['\"'] = "\\\"";
        }

        static inline const TRep& Instance() {
            return *Singleton<TRep>();
        }
    };
}

TString nicer(const TString& s) {
    const TRep& rep = TRep::Instance();
    TStringStream os;

    for (size_t i = 0; i < s.size(); ++i) {
        TRep::const_iterator r = rep.find(s[i]);

        if (r != rep.end())
            os << r->second;
        else
            os << s[i];
    }

    return os.Str();
}
