#include "lex.h"

namespace NHtmlLexer {
    void TLexer::OnToken(TResult* lio, HTLEX_TYPE type, unsigned len) {
        Y_ASSERT(Ctx.HTag || (type != HTLEX_START_TAG));
        Y_ASSERT(Ctx.HTag || (type != HTLEX_END_TAG));

        // postprocess attrs
        if (type == HTLEX_START_TAG && Ctx.AttCnt > 0) {
            // strip last /='/' attr - ignored by HTML5
            const NHtml::TAttribute& a = AttStore[Ctx.AttCnt - 1];
            if (a.Value.Start == 0 && a.Name.Leng == 1 && *(p_tokstart + a.Name.Start) == '/') {
                Ctx.AttCnt--;
            }

            for (unsigned i = 0; i < Ctx.AttCnt; ++i) {
                if (AttStore[i].Value.Start == 0) {
                    AttStore[i].Value = AttStore[i].Name;
                }
            }
        }

        unsigned cattr = lio->Attrs.size();
        lio->Attrs.insert(lio->Attrs.end(), AttStore, AttStore + Ctx.AttCnt);
        lio->AddToken(TToken(type, p_tokstart, len, namestart, namelen, Ctx.HTag,
                             cattr, Ctx.AttCnt, type == HTLEX_TEXT ? Ctx.Iws : false));

#ifdef HT_LDEBUG
        Cdbg << lio->Tokens.back() << Endl;
#endif
    }

    HTLEX_TYPE TObsoleteLexer::Next() {
        if (LexerResult.Tokens.empty()) {
            Y_ASSERT(Input.data() || Input.size() == 0);

            TLexer lex(Input.data(), Input.size());
            if (lex.Execute(&LexerResult) != 1)
                return HTLEX_EOF; // _ERROR;
        } else if (HaveToken())
            ++CToken;

        if (!HaveToken()) {
            return HTLEX_EOF;
        }

        const TToken& token = Token();

        return token.Type;
    }

    const char* TObsoleteLexer::getCurTag() {
        if (Token().Type == HTLEX_START_TAG)
            return (const char*)Token().Tag;
        if (Token().Type == HTLEX_END_TAG)
            return (const char*)Token().Tag + 1;
        return nullptr;
    }

    unsigned TObsoleteLexer::getCurTagLength() {
        if (Token().Type == HTLEX_START_TAG)
            return Token().TagLen;
        if (Token().Type == HTLEX_END_TAG)
            return Token().TagLen - 1;
        return 0;
    }

    bool TObsoleteLexer::getCurAttr(int idx, const char*& nameStart, int& nameLength, const char*& valStart, int& valLength) {
        NHtml::TAttribute* A = getCurAttr(idx);
        if (!A) {
            nameStart = nullptr;
            nameLength = 0;
            valStart = nullptr;
            valLength = 0;
            return false;
        }
        nameStart = GetCurText() + A->Name.Start;
        nameLength = A->Name.Leng;
        valStart = GetCurText() + A->Value.Start;
        valLength = A->Value.Leng;
        return true;
    }

    bool TObsoleteLexer::getCurAttrName(int idx, const char*& start, int& len) {
        NHtml::TAttribute* A = getCurAttr(idx);
        if (!A) {
            start = nullptr;
            len = 0;
            return false;
        }
        start = GetCurText() + A->Name.Start;
        len = A->Name.Leng;
        return true;
    }

    bool TObsoleteLexer::getCurAttrValue(int idx, const char*& start, int& len) {
        NHtml::TAttribute* A = getCurAttr(idx);
        if (!A) {
            start = nullptr;
            len = 0;
            return false;
        }
        start = GetCurText() + A->Value.Start;
        len = A->Value.Leng;
        return true;
    }

    char TObsoleteLexer::getCurAttrQuote(int idx) {
        NHtml::TAttribute* A = getCurAttr(idx);
        return (A) ? A->Quot : 0;
    }

        // Debug trace utilities

#ifndef HT_LDEBUG
    void TLexer::show_token_info(HTLEX_TYPE /*type*/, unsigned /*len*/) {
    }
    void TLexer::show_exec_results(const char* /*id*/) {
    }
    void TLexer::show_lex_pos(const char* /*id*/, const unsigned char* /*ptr*/) {
    }
    void TLexer::show_position(const char* /*id*/) {
    }
#else
    class TAttrsStreamer {
        const unsigned char* Token;
        const NHtml::TAttribute* Attrs;
        unsigned Count;

    public:
        TAttrsStreamer(const unsigned char* token, const NHtml::TAttribute* attrs, unsigned cnt)
            : Token(token)
            , Attrs(attrs)
            , Count(cnt)
        {
        }
        friend IOutputStream& operator<<(IOutputStream& os, const TAttrsStreamer& s);
    };

    void TLexer::show_token_info(HTLEX_TYPE type, unsigned len) {
        Cdbg << "token: " << type;
        if (type == HTLEX_START_TAG || type == HTLEX_END_TAG)
            (Cdbg << ", name: ").Write((const char*)namestart, namelen);
        // text: should be here
        Cdbg << ", text: [";
        Cdbg.Write((char*)p_tokstart, len);
        Cdbg << "]";

        // attrs
        if (type == HTLEX_START_TAG && Ctx.AttCnt)
            Cdbg << " attrs(" << Ctx.AttCnt << "): ["
                 << TAttrsStreamer(p_tokstart, AttStore, Ctx.AttCnt) << ']';

        Cdbg << Endl;
    }

    void TLexer::show_lex_pos(const char* id, const unsigned char* ptr) {
        if (!ptr)
            ptr = p;

        Cdbg << id << " at ";
        if (ptr == pe) {
            Cdbg << "end" << Endl;
            return;
        }
        Cdbg << ptr - p_tokstart << " (abs " << (size_t)ptr << ")" << Endl;
        Cdbg.Write((const char*)ptr, pe - ptr > 80 ? 80 : pe - ptr);
        Cdbg << (pe - ptr > 80 ? "..." : "") << Endl;
    }

    void TLexer::show_position(const char* id) {
        Cdbg << id << " at ";
        if (p == pe) {
            Cdbg << "end" << Endl;
            return;
        }
        Cdbg << " p=" << (size_t)p << ", pe=" << (size_t)pe << ", cs=" << cs << Endl;
        Cdbg.Write((const char*)p, pe - p > 80 ? 80 : pe - p);
        Cdbg << (pe - p > 80 ? "..." : "") << Endl;
    }

    IOutputStream& operator<<(IOutputStream& os, const TAttrsStreamer& s) {
        for (unsigned i = 0; i < s.Count; ++i) {
            // os << Attrs[i].Name
            os << ' '
               << s.Attrs[i].Name.Start << ',' << s.Attrs[i].Name.Leng
               << '='
               << s.Attrs[i].Value.Start << ',' << s.Attrs[i].Value.Leng;
            os << ' ';
            os.Write((const char*)(s.Token + s.Attrs[i].Name.Start),
                     s.Attrs[i].Name.Leng);
            os << "=";
            os.Write((const char*)s.Token + s.Attrs[i].Value.Start,
                     s.Attrs[i].Value.Leng);
            os << "";
        }
        return os;
    }

#endif // NDEBUG

}
