#pragma once

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <ysite/yandex/common/urltok.h>

class ITranslitTokenizer {
public:
    virtual void Tokenize(const TUtf16String& text) = 0;
    virtual ~ITranslitTokenizer() {
    }
};

class TNlpTranslitTokenizer: public ITranslitTokenizer {
    ITokenHandler& Handler;
public:
    TNlpTranslitTokenizer(ITokenHandler& handler)
        : Handler(handler)
    {}
private:
    void Tokenize(const TUtf16String& text) override {
        TNlpTokenizer tokenizer(Handler);
        tokenizer.Tokenize(text.c_str(), text.length());
    }
};

class TUrlTranslitTokenizer: public ITranslitTokenizer {
private:
    ITokenHandler& Handler;
public:
    TUrlTranslitTokenizer(ITokenHandler& handler)
        : Handler(handler)
    {}
private:
    void Tokenize(const TUtf16String& text) override {
            TString txt = WideToUTF8(text);
            TURLTokenizer tokenizer(txt.data(), txt.size());
        try {
            for (size_t i = 0; i < tokenizer.GetTokenCount(); ++i) {
                const size_t length = tokenizer.GetToken(i).size();
                const TChar* str = tokenizer.GetToken(i).data();
                TWideToken token(str, length);
                Handler.OnToken(token, length, GuessTypeByWord(str, length));
            }
        } catch (const ITokenHandler::TAllDoneException& a) {
        }
    }
};

class TSimpleTranslitTokenizer: public ITranslitTokenizer {
private:
    ITokenHandler& Handler;
public:
    TSimpleTranslitTokenizer(ITokenHandler& handler)
        : Handler(handler)
    {}
private:
    void Tokenize(const TUtf16String& text) override {
        try {
            size_t begin = 0;
            for (;;) {
                while (begin < text.length() && IsDelimiter(text[begin]))
                    ++begin;
                if (begin == text.length())
                    break;
                size_t len = 0;
                while (begin + len < text.length() && !IsDelimiter(text[begin + len]))
                    ++len;
                if (len) {
                    const TChar* str = text.c_str() + begin;
                    TWideToken token(str, len);
                    Handler.OnToken(token, len, GuessTypeByWord(str, len));
                }
                begin += len;
            }
        } catch (const ITokenHandler::TAllDoneException& a) {
        }
    }
    bool IsDelimiter(TChar c) {
        return (!IsAlpha(c) && !IsDigit(c) && c != '\'' && c != '\"' && c != '`');
    }
};
