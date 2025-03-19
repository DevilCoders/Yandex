#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>

#include <library/cpp/tokenizer/tokenizer.h>

#include "lemma_cache.h"

class TFormPureCalcer {
public:
    typedef TVector<TString> TForms;

private:
    class TTokenHandler;
    friend class TFormPureCalcer::TTokenHandler;

private:
    class TTokenHandler : public ITokenHandler {
    private:
        TFormPureCalcer* Owner;
        bool PairMode;
        bool Lemmatize;

        typedef TVector<TForms> TFormsVector;
        TFormsVector FormsVector;
        TForms SourceVector;

    public:
        TTokenHandler(TFormPureCalcer* owner, bool pairMode, bool lemmatize)
            : Owner(owner)
            , PairMode(pairMode)
            , Lemmatize(lemmatize)
        {
        }

        void Increment(const TString& key) {
            Owner->Forms.push_back(key);
        }

        void Flush() {
            if (PairMode) {
                for (size_t i = 1; i < FormsVector.size(); ++i)
                    for (size_t j = 0; j < FormsVector[i - 1].size(); ++j)
                        for (size_t k = 0; k < FormsVector[i].size(); ++k) {
                            Owner->Forms.push_back(FormsVector[i - 1][j] + "|" + FormsVector[i][k]);
                            Owner->Sources.push_back(SourceVector[i - 1] + "|" + SourceVector[i]);
                        }
            }
        }

        bool AddToken(const wchar16* src, size_t len, TLangMask langFlags, TCharCategory caseFlags, NLP_TYPE type) {
            const wchar16* dstToken;
            size_t dstLen;
            TCharTemp buffer(2*len);

            size_t derenyxedLen = Owner->Derenyx(langFlags, src, len, buffer.Data(), len);

            if (derenyxedLen > 0) {
                dstToken = buffer.Data();
                dstLen = derenyxedLen;
            } else {
                dstToken = src;
                dstLen = len;
            }

            ((wchar16*)dstToken)[dstLen] = 0;
            TUtf16String source = TUtf16String(dstToken, dstLen);
            TString key = Owner->ConvertToIndexFormat(source, caseFlags & CC_TITLECASE);
            TString firstLemma;
            if (!PairMode) {
                Increment(key);
                Owner->Sources.push_back( WideToUTF8(source) );
            }

            if (Lemmatize) {
                const TVector<TString>& lemmas = Owner->SimpleLemmatizationCache.SimpleLemmatize(dstToken, dstLen, type, langFlags);
                if (lemmas.size())
                    firstLemma = lemmas[0];
                SourceVector.push_back(key);
                if (PairMode)
                    FormsVector.push_back(TForms());
                for (size_t i = 0; i < lemmas.size(); ++i) {
                    const TString& lemma = lemmas[i];
                    if (!PairMode) {
                        Increment(lemma);
                    } else {
                        FormsVector.back().push_back(lemma);
                        if (FormsVector.back().size() > 2)
                            break;
                    }
                }
            }
            Owner->Lemmas.push_back(firstLemma);

            return false;
        }

        void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override {
            // Cout << TUtf16String(token.Token, token.Leng) << " " << type << Endl;
            if (type != NLP_WORD
                && type != NLP_INTEGER
                && type != NLP_FLOAT
                && type != NLP_MARK) {
                return;
            }

            if (token.SubTokens.size() == 0) { // a number
                const static TLangMask unk(LANG_RUS, LANG_ENG, LANG_UKR);
                const static TCharCategory lower = CC_LOWERCASE;
                AddToken(token.Token,
                         token.Leng,
                         unk,
                         lower, type);
                return;
            }

            NLemmer::TClassifiedMultiToken multitoken(token);

            for (size_t i = 0; i < multitoken.NumTokens(); ++i) {
                const TCharSpan& span = multitoken.Token(i);
                bool test = AddToken(multitoken.Str() + span.Pos,
                         span.Len,
                         multitoken.TokenAlphaLanguages(i),
                         multitoken.TokenAlphaCase(i),
                         type);
                 if (test) {
                    Cerr << "1-\t" << TUtf16String(multitoken.Str(), multitoken.Length()) << '\t' << multitoken.NumTokens() << '\t' << Endl;
                 }
            }

            if (multitoken.NumTokens() > 1 && Owner->PureMode) {
                AddToken(multitoken.Str(),
                         multitoken.Length(),
                         multitoken.CumulativeAlphaLanguage(),
                         multitoken.CumulativeAlphaCase(),
                         type);
            }
        }

        void Clear() {
            FormsVector.clear();
            SourceVector.clear();
        }
    };

    bool PairMode;
    bool Lemmatize;
    bool PureMode;
    TTokenHandler TokenHandler;
    TNlpTokenizer Tokenizer;
    TSimpleLemmatizationCache SimpleLemmatizationCache;

private:
    size_t Derenyx(TLangMask mask, const wchar16* src, size_t srclen, wchar16* buf, size_t buflen);
    TString ConvertToIndexFormat(const TUtf16String& form, bool isTitled);

public:
    TFormPureCalcer(bool pairMode, bool /*unused*/, bool lemmatize, bool pureMode)
        : PairMode(pairMode)
        , Lemmatize(lemmatize)
        , PureMode(pureMode)
        , TokenHandler(this, PairMode, Lemmatize)
        , Tokenizer(TokenHandler)
        , SimpleLemmatizationCache(PureMode)
    {
    }

    void Fill(const TUtf16String& text);

    TForms Forms;
    TForms Sources;
    TForms Lemmas;
};
