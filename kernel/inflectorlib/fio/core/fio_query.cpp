#include "fio_query.h"

#include "fio_exceptions.h"
#include "fio_inflector_core.h"
#include "fio_token.h"

#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/string_utils/scan/scan.h>
#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/core/lemmer.h>

#include <util/string/split.h>
#include <util/string/join.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/string/type.h>


namespace NFioInflector {

struct TQueryToken {
    TUtf16String Text;
    bool IsReserved = false;
    EGrammar Mark = gInvalid;
};

class TFioQueryHandler {
private:
    TVector<TQueryToken>* Tokens = nullptr;
    TFioInflectorOptions* Options = nullptr;

private:
    TString NormalizeKey(const TStringBuf& key) {
        TString norm = TString{key};
        StripInPlace(norm);
        return norm;
    }

    TString HandleOptionsString(TStringBuf value) {
        Y_ASSERT(Options);

        if (value.empty() || IsSpace(value)) {
            return {};
        }

        TSet<TStringBuf> optNames;

        for (const auto& it : StringSplitter(value).Split(',')) {
            TStringBuf token = it.Token();
            auto begin = token.begin();
            auto end = token.end();
            StripRange(begin, end);
            token = TStringBuf{begin, end};
            if (TStringBuf("advnorm") == token) {
                Options->SkipAdvancedNormaliation = false;
            } else {
                ythrow TFioInflectorException{}
                    << "unexpected option name: \"" << token << "\"";
            }

            optNames.insert(token);
        }

        return JoinSeq(",", optNames);
    }

public:
    TFioQueryHandler(
        TVector<TQueryToken>* tokens,
        TFioInflectorOptions* options)
        : Tokens(tokens)
        , Options(options)
    {
    }

    void operator() (const TStringBuf& key, const TStringBuf& value) {
        // save markup
        TQueryToken beg;
        beg.Text = UTF8ToWide(key) + L'{';
        Tokens->push_back(beg);

        TString normKey = NormalizeKey(key);

        // token processing
        TQueryToken token;
        TUtf16String normValue = UTF8ToWide(value);

        if (normKey == TStringBuf("gender")) {
            token.Mark = TGrammarIndex::GetCode(value);
            token.IsReserved = true;
        } else if (normKey == TStringBuf("opts")) {
            if (Options) {
                TString normOpts = HandleOptionsString(value);
                normValue = UTF8ToWide(normOpts);
            }
            token.IsReserved = true;
        } else {
            token.Mark = TGrammarIndex::GetCode(normKey);
        }
        token.Text = normValue;
        Tokens->push_back(token);

        // save markup
        TQueryToken end;
        end.Text += L'}';
        Tokens->push_back(end);
    }
};

class TFioTokenHandler : public ITokenHandler {
private:
    TVector<TFioToken>* Tokens;
    EGrammar CurState = gInvalid;
    bool NotAWord = false;

public:
    TFioTokenHandler(TVector<TFioToken>* tokens)
        : Tokens(tokens)
    {
    }

    void SetCurState(const TQueryToken& queryToken) {
        CurState = queryToken.Mark;
        NotAWord = queryToken.IsReserved;
    }

    void OnToken(const TWideToken& token, size_t origleng, NLP_TYPE type) override {
        Y_UNUSED(origleng);

        TFioToken fioToken(TUtf16String(token.Token, token.Leng), CurState);
        if (type == NLP_WORD && !NotAWord && CurState != gInvalid) {
            fioToken.IsWord = true;
        }
        Tokens->push_back(fioToken);
    }
};

void ParseQuery(const TWtringBuf& query, TVector<TQueryToken>* tokens, TFioInflectorOptions* options) {
    TFioQueryHandler fioQueryHandler(tokens, options);
    ScanKeyValue<true, '}', '{'>(WideToUTF8(query), fioQueryHandler);
}

void Tokenize(const TVector<TQueryToken>& query, TVector<TFioToken>* tokens) {
    TFioTokenHandler fioTokenHandler(tokens);
    TNlpTokenizer tokenizer(fioTokenHandler);

    for (const auto& token : query) {
        fioTokenHandler.SetCurState(token);
        tokenizer.Tokenize(token.Text);
    }
}

// Use only first gender field
EGrammar GetGender(const TVector<TFioToken>& tokens) {
    EGrammar gender = gMasFem;
    for (const auto& token : tokens) {
        if (token.Mark == gMasculine || token.Mark == gFeminine) {
            gender = token.Mark;
            return gender;
        }
    }
    return gender;
}

// Visible functions impl

TVector<TUtf16String> Inflect(const TLanguage* lang, const TUtf16String& query, const EGrammar cases[]) {
    TVector<TQueryToken> queryToks;
    TFioInflectorOptions options;

    ParseQuery(query, &queryToks, &options);

    TVector<TFioToken> fioToks;
    Tokenize(queryToks, &fioToks);

    EGrammar gender = GetGender(fioToks);

    TFioTokenInflector queryInflector(lang, options);
    if (gender == gMasFem) {
        gender = queryInflector.GuessGender(fioToks);
    }

    queryInflector.LemmatizeFio(&fioToks, gender);

    TVector<TUtf16String> inflections;
    for (size_t i = 0; cases[i] != gInvalid; ++i) {
        TUtf16String inflection = queryInflector.InflectInCase(fioToks, cases[i]);
        inflections.push_back(inflection);
    }

    return inflections;
}

bool HasMarkup(const TUtf16String& query) {
    bool hasKeyValSep = query.find(L'{') != TUtf16String::npos;
    bool hasFieldSep = query.find(L'}') != TUtf16String::npos;
    return hasKeyValSep && hasFieldSep;
}

} // namespace NFioInflector
