#include "words_messer.h"

#include <kernel/snippets/config/config.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/alpha/abc.h>

#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/algorithm.h>
#include <util/random/shuffle.h>

namespace NSnippets {

    struct TMutateTokenHandler : public ITokenHandler {
        TWtringBuf Origin;
        TVector<TWtringBuf> Parts;
        TVector<bool> Word;
        TVector<bool> BoldWord;
        TVector<bool> CapWord;
        TVector<bool> GWord;
        int Bal;
        TMutateTokenHandler()
          : Bal(0)
        {
        }
        void OnToken(const TWideToken& /*multiToken*/, size_t origleng, NLP_TYPE type) override {
            bool word = false;
            bool capWord = false;
            bool gWord = false;
            if (false
                || type == NLP_WORD
                || type == NLP_MARK
                || type == NLP_INTEGER
                || type == NLP_FLOAT
            ) {
                word = true;
            } else {
                word = false;
            }
            TWtringBuf s = Origin.NextTokAt(origleng);
            for (size_t i = 0; i + 1 < s.size(); ++i) {
                if (s[i] == '\x07') {
                    if (s[i + 1] == '[') {
                        ++Bal;
                    } else if (s[i + 1] == ']') {
                        --Bal;
                    }
                }
            }
            if (word && s.size() && ToUpper(s[0]) == s[0] && IsAlpha(s[0])) {
                capWord = true;
            }
            if (word) {
                TLangMask langMask = LI_BASIC_LANGUAGES;
                TWLemmaArray lemmas;
                NLemmer::AnalyzeWord(s.data(), s.size(), lemmas, langMask);
                TGramBitSet gram = TGramBitSet::FromBytes(lemmas[0].GetStemGram());
                if (gram.Has(gConjunction) || gram.Has(gParticle) || gram.Has(gPreposition) || gram.Has(gInterjunction)) {
                    gWord = true;
                }
            }
            Word.push_back(word);
            BoldWord.push_back(word && Bal > 0);
            CapWord.push_back(capWord);
            GWord.push_back(gWord);
            Parts.push_back(s);

        }
    };

    static void Mutate(TUtf16String& s, TMersenne<ui64>& rnd, bool bold, bool reg, bool gg, size_t pct) {
        TMutateTokenHandler h;
        h.Origin = s;
        TNlpTokenizer t(h, false);
        t.Tokenize(s.data(), s.size());
        TVector<bool> fixed(h.Parts.size(), false);
        TVector<size_t> p;
        for (size_t i = 0; i < h.Parts.size(); ++i) {
            p.push_back(i);
            if (rnd.GenRand() % 100 >= pct || !h.Word[i] || !bold && h.BoldWord[i] || !reg && !h.BoldWord[i] || !gg && h.GWord[i]) {
                fixed[i] = true;
            }
        }
        Shuffle(p.begin(), p.end(), rnd);
        TUtf16String res;
        size_t j = 0, jb = 0;
        for (size_t i = 0; i < p.size(); ++i) {
            if (fixed[i]) {
                res += h.Parts[i];
                continue;
            }
            Y_ASSERT(h.Word[i]);
            TWtringBuf q;
            if (h.BoldWord[i]) {
                while (!h.BoldWord[p[jb]] || fixed[p[jb]]) {
                    ++jb;
                }
                q = h.Parts[p[jb++]];
            } else {
                while (!h.Word[p[j]] || h.BoldWord[p[j]] || fixed[p[j]]) {
                    ++j;
                }
                q = h.Parts[p[j++]];
            }
            if (h.CapWord[i] && q.size() && ToUpper(q[0]) != q[0]) {
                res += ToUpper(q[0]);
                res += TWtringBuf(q.data() + 1, q.data() + q.size());
            } else {
                res += q;
            }
        }
        s = res;
    }

    void TWordShuffler::ShuffleWords(TUtf16String& w) {
        Mutate(w, Rnd, true, true, true, 100);
    }

    void TWordShuffler::ShuffleWords(const TConfig& cfg, TUtf16String& w) {
        size_t prob = cfg.BrkProb();
        if (Rnd.GenRand() % 100 >= prob) {
            return;
        }
        bool bold = cfg.BrkBold();
        bool reg = cfg.BrkReg();
        bool gg = cfg.BrkGg();
        size_t pct = cfg.BrkPct();
        Mutate(w, Rnd, bold, reg, gg, pct);
    }
}
