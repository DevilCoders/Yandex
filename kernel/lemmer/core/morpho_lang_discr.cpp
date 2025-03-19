#include "morpho_lang_discr.h"
#include "wordinstance.h"
#include <library/cpp/wordlistreader/wordlistreader.h>
#include <util/string/split.h>

static bool IsVowel(wchar16 c) {
    return NLemmer::GetCharInfoAlphaRequired(c, true).any();
}

namespace NMorphoLangDiscriminator {
    const TContext TContext::EmptyContext;
    const TContext::TVQual TContext::EmptyQual;

    class TContext::TContextLoader: public TWordListReader {
    private:
        TQuality CurrentQuality;
        TContext& Ctx;

    private:
        void ParseLine(const TUtf16String& line, ELanguage langcode, int version) override;

    public:
        TContextLoader(TContext& ctx, const char* fileName)
            : CurrentQuality(QGood)
            , Ctx(ctx)
        {
            ReadDataFile(fileName);
        }

        TContextLoader(TContext& ctx, IInputStream& dataFile)
            : CurrentQuality(QGood)
            , Ctx(ctx)
        {
            ReadDataFile(dataFile);
        }
    };

    void TContext::TContextLoader::ParseLine(const TUtf16String& line, ELanguage langcode, int version) {
        static const TUtf16String delimiters = u" \t\r\n,;";
        static const TUtf16String strBad = u"BAD:";
        static const TUtf16String strAverage = u"AVERAGE:";
        static const TUtf16String strPref = u"PREFIXOID:";
        static const TUtf16String strAlmostGood = u"ALMOSTGOOD:";
        static const TUtf16String strGood = u"GOOD:";
        static const TUtf16String strName = u"NAME:";
        static const TUtf16String strGoodFreq = u"GOODFREQ:";

        TVector<TUtf16String> tokens;
        StringSplitter(line).SplitBySet(delimiters.c_str()).SkipEmpty().Collect(&tokens);
        TVector<TUtf16String>::iterator it;
        for (it = tokens.begin(); it != tokens.end(); it++) {
            if (it->empty())
                continue;      // due diligence
            if (version > 1) { // support for stickiness comes from version 2
                if (*it == strBad) {
                    CurrentQuality = QBad;
                    continue;
                } else if (*it == strAverage) {
                    CurrentQuality = QAverage;
                    continue;
                } else if (*it == strPref) {
                    CurrentQuality = QPref;
                    continue;
                } else if (*it == strAlmostGood) {
                    CurrentQuality = QAlmostGood;
                    continue;
                } else if (*it == strGood) {
                    CurrentQuality = QGood;
                    continue;
                } else if (*it == strName) {
                    CurrentQuality = QName;
                    continue;
                } else if (*it == strGoodFreq) {
                    CurrentQuality = QGoodFreq;
                    continue;
                }
            }
            Ctx.Words[*it][langcode] = CurrentQuality;
        }
    }

    bool TContext::Init(const char* fileName) {
        TContextLoader(*this, fileName);
        return true;
    }

    bool TContext::Init(IInputStream& dataFile) {
        TContextLoader(*this, dataFile);
        return true;
    }

    bool IsGoodBut(const TLemmaForms& lm) {
        static const TLangMask rusLang(LANG_RUS, LANG_UKR);
        return rusLang.SafeTest(lm.GetLanguage()) && (lm.HasFlexGram(gImperative) || lm.HasFlexGram(gShort));
    }

    TQuality GetLemmaQuality(const TLemmaForms& lm, const TUtf16String& form) {
        static const TGramBitSet cg(gConjunction, gPreposition);
        static const TGramBitSet name(gFirstName, gSurname, gPatr);

        if (!lm.IsBastard()) {
            if (lm.HasGram(gObsolete))
                return QAverage;
            if (lm.GetStemGrammar().HasAny(cg)) //should be a direct word list here
                return QGoodFreq;
            bool nm = lm.GetStemGrammar().HasAny(name);
            if (nm && lm.GetLemma() == form)
                return QName;
            if (IsGoodBut(lm))
                return QAlmostGood;
            if (lm.IsBest() && !nm)
                return QVeryGood;
            return QGood;
        }

        if (lm.GetQuality() & TYandexLemma::QPrefixoid) { // bastard or sob
            if (lm.HasGram(gObsolete))
                return QAverage;
            return QPref;
        }

        if (lm.GetQuality() & TYandexLemma::QFoundling)
            return QBad;

        Y_ASSERT(lm.GetQuality() & (TYandexLemma::QBastard | TYandexLemma::QSob));
        if (lm.HasGram(gObsolete))
            return QBad;
        return QAverage;
    }

    void GetQuality(const TWordInstance& word, TLang2Qual& ret, const TContext& ctx) {
        ret.clear();
        const TContext::TVQual& fixMask = ctx.GetMask(word.GetNormalizedForm());
        for (TWordInstance::TLemmasVector::const_iterator i = word.GetLemmas().begin(); i != word.GetLemmas().end(); ++i) {
            ELanguage lang = i->GetLanguage();
            if (!lang)
                continue;
            TQuality qual = fixMask[lang];
            if (!qual)
                qual = GetLemmaQuality(*i, word.GetNormalizedForm());
            if (ret[lang] < qual)
                ret[lang] = qual;
        }
    }

    bool HasaDiamond(TQuality q1, TQuality q2) {
        switch (q1) {
            case QGoodFreq:
            case QVeryGood:
            case QGood:
            case QAlmostGood:
            case QPref:
                return q2 == QAverage || q2 == QBad;
            case QName:
            case QAverage:
                return q2 == QBad;
            default:
                return false;
        }
    }

    bool HasVowel(const TUtf16String& word) {
        for (size_t i = 1; i < word.length(); ++i) {
            if (IsVowel(word[i]))
                return true;
        }
        return false;
    }

    int GetDiamonds(TQuality q1, TQuality q2, const TUtf16String& form, bool isTheOnlyWord) {
        if (form.length() <= 1)
            return 0;
        if (!HasaDiamond(q1, q2))
            return 0;
        if (!isTheOnlyWord && q1 == QGoodFreq && (q2 == QAverage || q2 == QBad))
            return 2;
        if (form.length() <= 2)
            return 0;
        if ((q1 == QGoodFreq || q1 == QGood || q1 == QName || q1 == QVeryGood || q1 == QAlmostGood) && (q2 == QBad) && form.length() > 3 && HasVowel(form))
            return 2;
        return 1;
    }

    int GetRubies(TQuality q1, TQuality q2, const TUtf16String& form) {
        if (form.length() <= 1)
            return 0;
        if (q1 == QVeryGood && q2 == QAlmostGood)
            return 1;
        return 0;
    }

    int GetZeroScore(TQuality q, const TUtf16String& form, bool isTheOnlyWord) {
        if (q > QBad && form.length() > 3 && HasVowel(form) || q == QGoodFreq && form.length() > 1 && !isTheOnlyWord) {
            return 2;
        }
        return 1;
    }
}

using NMorphoLangDiscriminator::TLang2Qual;

void TMorphoLangDiscriminator::AddDiamonds(const TLang2Qual& qual, const TUtf16String& form, bool isTheOnlyWord) {
    using NMorphoLangDiscriminator::GetDiamonds;

    for (const auto& i : qual) {
        AllLanguages.Set(i.first);
        int zi = GetZeroScore(i.second, form, isTheOnlyWord);
        Intersection[i.first][i.first] += zi;
        if (qual.size() > 1) {
            IntersectedLanguages.Set(i.first);
        }
        for (const auto& j : qual) {
            if (i.first == j.first) {
                continue;
            }
            Intersection[i.first][j.first] += zi;
            size_t rd = GetDiamonds(i.second, j.second, form, isTheOnlyWord);
            size_t rr = GetRubies(i.second, j.second, form);
            Diamonds[i.first][j.first] += rd;
            Rubies[i.first][j.first] += rr;
        }
    }
}

TMorphoLangDiscriminator::TMorphoLangDiscriminator(const NMorphoLangDiscriminator::TContext& ctx, TLangMask keepLanguages)
    : Context(ctx)
    , KeepLanguages(keepLanguages)
{
    memset(Diamonds, 0, sizeof(Diamonds));
    memset(Rubies, 0, sizeof(Rubies));
    memset(Intersection, 0, sizeof(Intersection));
}

void TMorphoLangDiscriminator::AddWord(const TWordInstance& word, bool isTheOnlyWord) {
    THashSet<TUtf16String>::const_iterator i = Words.find(word.GetNormalizedForm());
    if (i != Words.end())
        return;
    Words.insert(word.GetNormalizedForm());
    TLang2Qual qual;
    GetQuality(word, qual, Context);
    AddDiamonds(qual, word.GetNormalizedForm(), isTheOnlyWord);
}

static ui16 Barrier(ELanguage l1, ELanguage l2, TLangMask mask) {
    return !mask.Test(l1) || mask.Test(l2);
}

TMorphoLangDiscriminator::TResult TMorphoLangDiscriminator::ObtainResult(TLangMask preferedLanguage) const {
    TLangMask ret;
    for (ELanguage i : IntersectedLanguages) {
        for (ELanguage j : IntersectedLanguages) {
            if (i == j) {
                continue;
            }
            if (!Intersection[i][j]) {
                Y_ASSERT(!Intersection[j][i]);
                continue;
            }
            Y_ASSERT(Intersection[i][j] <= Intersection[i][i]);

            ui16 dij = Diamonds[i][j] + (Intersection[i][i] - Intersection[i][j]);
            ui16 dji = Diamonds[j][i] + (Intersection[j][j] - Intersection[j][i]);

            if (dji) {
                continue;
            }

            if (dij > Barrier(i, j, preferedLanguage)) {
                ret.Set(j);
            } else {
                if (Rubies[i][j] > 1 && !Rubies[j][i]) {
                    ret.Set(j);
                } else {
                    if ((Rubies[j][i] <= 1 || Rubies[i][j]) && preferedLanguage.Test(i) && !preferedLanguage.Test(j)) {
                        ret.Set(j);
                    }
                }
            }
        }
    }

    ret &= ~KeepLanguages;
    return TResult(ret, AllLanguages & ~ret);
}
