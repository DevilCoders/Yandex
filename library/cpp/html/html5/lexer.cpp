#include "lexer.h"

#include <util/system/yassert.h>

namespace NHtml5 {
    TLexer::TLexer(const char* b, size_t len)
        : Tokenizer_(TTokenizerOptions(), b, len)
        , More_(true)
    {
        InitializeText();
    }

    bool TLexer::More() const {
        return More_;
    }

    int TLexer::Execute(NHtmlLexer::TResult* lio) {
        Y_ASSERT(lio->Tokens.empty());

        while (true) {
            TToken token;

            Tokenizer_.Lex(&token);

            switch (token.Type) {
                case TOKEN_DOCTYPE:
                    MaybeEmitTextToken(lio);
                    EmitDoctypeTag(token, lio);
                    break;
                case TOKEN_START_TAG:
                    MaybeEmitTextToken(lio);
                    EmitStartTag(token, lio);
                    MaybeChangeTokenizerState(token);
                    break;
                case TOKEN_END_TAG:
                    MaybeEmitTextToken(lio);
                    EmitEndTag(token, lio);
                    break;
                case TOKEN_COMMENT:
                    MaybeEmitTextToken(lio);
                    EmitCommentToken(token, lio);
                    break;
                case TOKEN_WHITESPACE:
                case TOKEN_CHARACTER:
                    if (Text_.Length == 0) {
                        Text_.Type = token.Type;
                        Text_.Start = token.OriginalText.Data;
                    }
                    if (token.Type == TOKEN_CHARACTER) {
                        Text_.Type = TOKEN_CHARACTER;
                    }
                    Text_.Length++;
                    break;
                case TOKEN_NULL:
                    MaybeEmitTextToken(lio);
                    break;
                case TOKEN_EOF:
                    MaybeEmitTextToken(lio);
                    More_ = false;
                    break;
            }

            if (!More_) {
                return 0;
            }
        }
        return 0;
    }

    void TLexer::EmitCommentToken(const TToken& token, NHtmlLexer::TResult* lio) {
        NHtmlLexer::TToken res;
        res.Type = HTLEX_COMMENT;
        res.Text = token.OriginalText.Data;
        res.Leng = token.OriginalText.Length;
        lio->AddToken(res);
    }

    void TLexer::EmitDoctypeTag(const TToken& token, NHtmlLexer::TResult* lio) {
        NHtmlLexer::TToken res;
        res.Type = HTLEX_MD;
        res.Text = token.OriginalText.Data;
        res.Leng = token.OriginalText.Length;
        lio->AddToken(res);
    }

    void TLexer::EmitEndTag(const TToken& token, NHtmlLexer::TResult* lio) {
        NHtmlLexer::TToken res;
        res.Type = HTLEX_END_TAG;
        res.NAtt = 0;
        res.Attrs = nullptr;
        FinishTag(token, &res);
        lio->AddToken(res);
    }

    void TLexer::EmitStartTag(const TToken& token, NHtmlLexer::TResult* lio) {
        NHtmlLexer::TToken res;
        res.Type = HTLEX_START_TAG;
        res.AttStart = lio->Attrs.size();
        res.NAtt = 0;
        res.Attrs = nullptr;
        FinishTag(token, &res);
        // Add attributes
        for (size_t i = 0; i < token.v.StartTag.Attributes.Length; ++i) {
            NHtml::TAttribute attr;
            const NHtml5::TAttribute& oldAttr = token.v.StartTag.Attributes.Data[i];

            attr.Namespace = EAttrNS::NONE;
            attr.Name.Start = oldAttr.OriginalName.Data - res.Text;
            attr.Name.Leng = oldAttr.OriginalName.Length;

            if (oldAttr.OriginalName.Data == oldAttr.OriginalValue.Data) {
                attr.Value.Start = attr.Name.Start;
                attr.Value.Leng = attr.Name.Leng;
            } else {
                if (oldAttr.OriginalValue.Length > 1 && (oldAttr.OriginalValue.Data[0] == '\'' || oldAttr.OriginalValue.Data[0] == '\"')) {
                    attr.Quot = oldAttr.OriginalValue.Data[0];
                    attr.Value.Start = oldAttr.OriginalValue.Data - res.Text + 1;
                    attr.Value.Leng = oldAttr.OriginalValue.Length - 2;
                } else {
                    attr.Quot = '\0';
                    attr.Value.Start = oldAttr.OriginalValue.Data - res.Text;
                    attr.Value.Leng = oldAttr.OriginalValue.Length;
                }
            }

            res.NAtt++;
            lio->Attrs.push_back(attr);
        }
        lio->AddToken(res);
    }

    void TLexer::FinishTag(const TToken& token, NHtmlLexer::TToken* res) {
        res->Text = token.OriginalText.Data;
        res->Leng = token.OriginalText.Length;

        {
            const TStringPiece name = GetTagFromOriginalText(token.OriginalText);

            res->Tag = (const unsigned char*)name.Data;
            res->TagLen = name.Length;
            res->HTag = &NHtml::FindTag(name.Data, name.Length);
        }
    }

    void TLexer::InitializeText() {
        Text_.Type = TOKEN_WHITESPACE;
        Text_.Start = nullptr;
        Text_.Length = 0;
    }

    void TLexer::MaybeChangeTokenizerState(const TToken& token) {
        Y_ASSERT(token.Type == TOKEN_START_TAG);

        switch (token.v.StartTag.Tag) {
            case TAG_SCRIPT:
                Tokenizer_.SetState(LEX_SCRIPT);
                break;
            case TAG_PLAINTEXT:
                Tokenizer_.SetState(LEX_PLAINTEXT);
                break;
            case TAG_TEXTAREA:
                Tokenizer_.SetState(LEX_RCDATA);
                break;
            case TAG_STYLE:
            case TAG_XMP:
                Tokenizer_.SetState(LEX_RAWTEXT);
                break;
            default:
                break;
        }
    }

    void TLexer::MaybeEmitTextToken(NHtmlLexer::TResult* lio) {
        if (Text_.Length == 0) {
            return;
        }

        NHtmlLexer::TToken res;
        res.Type = HTLEX_TEXT;
        res.IsWhitespace = (Text_.Type == TOKEN_WHITESPACE);
        res.Text = Text_.Start;
        res.Leng = Text_.Length;
        lio->AddToken(res);

        InitializeText();
    }

}
