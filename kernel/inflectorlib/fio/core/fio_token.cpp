#include "fio_token.h"

#include "fio_exceptions.h"

#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/core/lemmer.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/string/split.h>
#include <util/string/vector.h>

namespace {
    // TODO: May be replaced with kernel/inflectorlib/phrase/complexword.h
    // TComplexWord::CopyCapitalization
    void RestoreCase(TCharCategory flag, bool startsFromUpper, TUtf16String* text) {
        if (flag & CC_TITLECASE) {
            text->to_title();
        } else if (flag & CC_UPPERCASE) {
            text->to_upper();
        } else if (flag & CC_MIXEDCASE) {
            // anna-bogdana -> Anna-Bogdana
            static const TUtf16String DELIM = ToWtring("-");
            if (startsFromUpper && text->find(DELIM) != TUtf16String::npos) {
                TVector<TUtf16String> subTokens;
                StringSplitter(*text).SplitByString(DELIM.c_str()).SkipEmpty().Collect(&subTokens);
                for (auto& token : subTokens) {
                    token.to_title();
                }
                *text = JoinStrings(subTokens, DELIM);
            }
        }
    }

    bool IsDiacriticsExclusion(const TUtf16String& sample, const TUtf16String& text) {
        static const TUtf16String exclusion = u"пётр"; // The only Russian name that loses ё in inflected forms

        TUtf16String lowercased(sample);
        lowercased.to_lower();
        if (lowercased == exclusion && text.size() != lowercased.size()) // non-nominative case
            return true;
        return false;
    }

    TUtf16String RestoreDiacritics(const TUtf16String& sample, const TUtf16String& text) {
        if (IsDiacriticsExclusion(sample, text)) {
            return text;
        }
        TVector<wchar16> result(text.begin(), text.end());
        for (size_t i = 0; i < sample.size() && i < text.size(); ++i) {
            if (sample[i] == 0x0401 && result[i] == 0x0415) // Е -> Ё
                result[i] = sample[i];
            else if (sample[i] == 0x0451 && result[i] == 0x0435) // е -> ё
                result[i] = sample[i];
            else if (sample[i] != result[i])
                break;
        }
        return TUtf16String(result.begin(), result.end());
    }

    class TFioTextTokenHandler : public ITokenHandler {
    private:
        TVector<NFioInflector::TFioToken>* Tokens;

    public:
        TFioTextTokenHandler(TVector<NFioInflector::TFioToken>* tokens)
            : Tokens(tokens)
        {
        }

        void OnToken(const TWideToken& token, size_t origleng, NLP_TYPE type) override {
            Y_UNUSED(origleng);

            NFioInflector::TFioToken fioToken(TUtf16String(token.Token, token.Leng), gInvalid);
            if (type == NLP_WORD) {
                fioToken.IsWord = true;
            }
            Tokens->push_back(fioToken);
        }
    };

}

namespace NFioInflector {
    EGrammar TFioTokenInflector::GuessGender(const TVector<TFioToken>& tokens, EGrammar proper) const {
        EGrammar gender = gMasFem;
        for (const auto& token : tokens) {
            if (!token.IsWord || token.Mark != proper) {
                continue;
            }

            EGrammar cur = Inflector.GuessGender(token.Text, token.Mark);
            if (cur == gMasFem) {
                continue;
            }

            if (gender == gMasFem) {
                gender = cur;
            }

            if (cur != gender) {
                gender = gMasFem;
                break;
            }
        }

        return gender;
    }

    EGrammar TFioTokenInflector::GuessGender(const TVector<TFioToken>& tokens) const {
        EGrammar nameGender = GuessGender(tokens, gFirstName);
        // EGrammar patrnGender = GuessGender(tokens, gPatr);
        EGrammar surnameGender = GuessGender(tokens, gSurname);

        if (nameGender == surnameGender || nameGender != gMasFem) {
            return nameGender;
        }

        if (surnameGender != gMasFem) {
            return surnameGender;
        }

        return GuessGender(tokens, gInvalid);
    }

    void TFioTokenInflector::LemmatizeFio(TVector<TFioToken>* tokens, EGrammar gender) const {
        for (auto& token : *tokens) {
            if (!token.IsWord) {
                continue;
            }

            try {
                token.Lemmas = Inflector.Analyze(token.Text, gender, token.Mark);
            }
            catch (const TAnalyzeException&) {
            }

            token.CharCategory = NLemmer::ClassifyCase(token.Text.data(), token.Text.length());

            if (!token.Text.empty() && IsUpper(token.Text[0])) {
                token.StartsFromUpperCase = true;
            }
        }
    }

    TUtf16String TFioTokenInflector::InflectInCase(const TVector<TFioToken>& tokens, EGrammar gCase) const {
        TUtf16String result;
        for (const auto& token : tokens) {
            if (token.Lemmas.empty()) {
                result += token.Text;
                continue;
            }

            TUtf16String inflection = Inflector.Generate(token.Lemmas, gCase);
            RestoreCase(token.CharCategory, token.StartsFromUpperCase, &inflection);
            inflection = RestoreDiacritics(token.Text, inflection);
            result += inflection;
        }

        return result;
    }

    void TFioTokenInflector::SetUnsetProper(TVector<TFioToken>* tokens, const TGramBitSet& hint) const {
        for (auto& token : *tokens) {
            if (!token.IsWord) {
                continue;
            }
            token.Mark = Inflector.GuessProper(token.Text, hint);
        }
    }

    void TokenizeText(const TUtf16String& text, TVector<TFioToken>* tokens) {
        TFioTextTokenHandler fioTokenHandler(tokens);
        TNlpTokenizer tokenizer(fioTokenHandler);

        tokenizer.Tokenize(text);
    }


} // namespace NFioInflector
