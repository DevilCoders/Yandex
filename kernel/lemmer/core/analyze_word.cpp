#include <util/generic/strbuf.h>

#include "language.h"
#include "lemmeraux.h"

namespace NLemmer {
    namespace NDetail {
        namespace {
            struct TSubTokenCoord {
                size_t Begin;
                size_t End;
                bool IsMultitoken;

                TSubTokenCoord(size_t begin, size_t end, bool isMult)
                    : Begin(begin)
                    , End(end)
                    , IsMultitoken(isMult)
                {
                }

                TSubTokenCoord(size_t theOnly)
                    : Begin(theOnly)
                    , End(theOnly + 1)
                    , IsMultitoken(false)
                {
                }

                size_t Length() const {
                    return End - Begin;
                }
            };

            enum EAnalyzeWordMode {
                awModeNormal = 0,
                awModeTranslit,
                awModeTranslate
            };

            class TLangAndMode {
            private:
                ELanguage Id_;
                const TLanguage* Lang;
                EAnalyzeWordMode TransMod;
                ELanguage AlphaLangId_;
                const TLanguage* LangBreak_;

            public:
                TLangAndMode(const ELanguage lang, EAnalyzeWordMode transMode)
                    : Id_(lang)
                    , Lang(NLemmer::GetLanguageById(Id_))
                    , TransMod(transMode)
                    , AlphaLangId_(lang)
                    , LangBreak_(TransMod == awModeTranslate ? NLemmer::GetLanguageById(LANG_ENG) : Lang)
                {
                }

                ELanguage Id() const {
                    return Id_;
                }

                const TLanguage* Language() const {
                    return Lang;
                }

                EAnalyzeWordMode TransMode() const {
                    return TransMod;
                }

                bool IsForeign() const {
                    return !!TransMod;
                }

                ELanguage AlphaLangId() const {
                    return AlphaLangId_;
                }

                const TLanguage* LangBreak() const {
                    return LangBreak_;
                }
            };

            // Inner typedef for analysis coverage quality
            class TCoverage {
            private:
                typedef ui16 ECoverageLevel;

                static const ECoverageLevel CL_NONE;              /* = 0;*/
                static const ECoverageLevel CL_PRIMARY;           /* = 32768;*/
                static const ECoverageLevel CL_FOUNDLING;         /* = 1;*/
                static const ECoverageLevel CL_BASTARD;           /* = 2;*/
                static const ECoverageLevel CL_REGULAR;           /* = 3;*/
                static const ECoverageLevel CL_PRIMARY_FOUNDLING; /* = CL_FOUNDLING | CL_PRIMARY;*/
                static const ECoverageLevel CL_PRIMARY_BASTARD;   /* = CL_BASTARD | CL_PRIMARY;*/
                static const ECoverageLevel CL_PRIMARY_REGULAR;   /* = CL_REGULAR | CL_PRIMARY;*/

                ECoverageLevel Coverage[MAX_SUBTOKENS];
                size_t NumTokens;
                const NLemmer::TAnalyzeWordOpt& Opt;
                bool IsPrimaryState;

            public:
                TCoverage(size_t numTokens, const NLemmer::TAnalyzeWordOpt& opt)
                    : NumTokens(numTokens)
                    , Opt(opt)
                    , IsPrimaryState(false)
                {
                    Y_ASSERT(NumTokens <= MAX_SUBTOKENS);
                    Clear();
                }

                void SetPrimaryState(bool isPrimary) {
                    IsPrimaryState = isPrimary;
                }

                void AddLemma(const TYandexLemma& lm) {
                    const ECoverageLevel quality = LemmaQuality(lm);
                    Cover(lm.GetTokenPos(), lm.GetTokenPos() + lm.GetTokenSpan(), quality);
                }

                void Clear() {
                    for (size_t i = 0; i < NumTokens; ++i) {
                        Coverage[i] = CL_NONE;
                    }
                }
                bool IsCovered(size_t i) const {
                    Y_ASSERT(i < NumTokens);
                    return Coverage[i] > CL_NONE;
                }

                NLemmer::TLanguageQualityAccept AcceptMode(const TLangAndMode& lam, const TSubTokenCoord& sc) const {
                    const ECoverageLevel minimumQuality = MinimumQuality(sc.Begin, sc.End);
                    if (minimumQuality == CL_PRIMARY_REGULAR)
                        return NLemmer::TLanguageQualityAccept(); // the chunk is already covered by first-grade analysis from a higer-rank primary language

                    if (lam.IsForeign())
                        return GetEnglishQuality(lam.Language()->Id);
                    return GetOwnQuality(lam.Language()->Id, minimumQuality, sc.IsMultitoken);
                }

                bool ShouldReset(const TYandexLemma& lm) const {
                    if (!ShouldReset_(lm.GetLanguage()))
                        return false;
                    if (lm.GetQuality() & (TYandexLemma::QFromEnglish | TYandexLemma::QToEnglish)) // do not reset translated
                        return false;

                    const ui32 qual = lm.GetQuality() & TYandexLemma::QAnyBastard;

                    if (!qual) // dictionary
                        return !Opt.AcceptDictionary.SafeTest(lm.GetLanguage());
                    if (qual & TYandexLemma::QSob)
                        return !Opt.AcceptSob.SafeTest(lm.GetLanguage());
                    if (qual & TYandexLemma::QBastard)
                        return !Opt.AcceptBastard.SafeTest(lm.GetLanguage());
                    return false; // foundling;
                }

                bool AnyCovered() const {
                    for (size_t i = 0; i < NumTokens; ++i) {
                        if (IsCovered(i))
                            return true;
                    }
                    return false;
                }

            private:
                ECoverageLevel LemmaQuality(const TYandexLemma& lm) const {
                    ECoverageLevel quality = (lm.GetQuality() & TYandexLemma::QFoundling) ? CL_FOUNDLING
                                                                                          : lm.IsBastard() ? CL_BASTARD : CL_REGULAR;
                    if (IsPrimaryState)
                        quality |= CL_PRIMARY;
                    return quality;
                }

                void Cover(size_t begin, size_t end, ECoverageLevel quality) {
                    Y_ASSERT(begin < NumTokens);
                    Y_ASSERT(end <= NumTokens);
                    for (size_t k = begin; k < end; ++k)
                        Coverage[k] = Max(Coverage[k], quality);
                }

                ECoverageLevel MinimumQuality(size_t begin, size_t end) const {
                    Y_ASSERT(begin < NumTokens);
                    Y_ASSERT(end <= NumTokens);
                    ECoverageLevel minimumQuality = CL_PRIMARY_REGULAR;
                    for (size_t idx = begin; idx < end; idx++)
                        if (Coverage[idx] < minimumQuality)
                            minimumQuality = Coverage[idx];
                    return minimumQuality;
                }

                NLemmer::TLanguageQualityAccept GetOwnQuality(ELanguage lang, ECoverageLevel minimumQuality, bool isMultiToken) const {
                    NLemmer::TLanguageQualityAccept acc;
                    const bool acceptAnyway = ShouldReset_(lang);

                    if (Opt.AcceptDictionary.SafeTest(lang) || acceptAnyway)
                        acc.Set(NLemmer::AccDictionary);

                    // Accept bastards only for single-token words, and only if no bastards are produced by primary languages
                    if (isMultiToken)
                        return acc;

                    if (minimumQuality < CL_PRIMARY_BASTARD) {
                        if (Opt.AcceptSob.SafeTest(lang) || acceptAnyway)
                            acc.Set(NLemmer::AccSob);
                        if (Opt.AcceptBastard.SafeTest(lang) || acceptAnyway)
                            acc.Set(NLemmer::AccBastard);
                    }
                    if (minimumQuality < CL_PRIMARY_FOUNDLING && Opt.AcceptFoundling.SafeTest(lang) || lang == LANG_UNK)
                        acc.Set(NLemmer::AccFoundling);
                    return acc;
                }

                NLemmer::TLanguageQualityAccept GetEnglishQuality(ELanguage lang) const {
                    NLemmer::TLanguageQualityAccept acc;

                    if (Opt.AcceptFromEnglish.SafeTest(lang))
                        acc.Set(NLemmer::AccFromEnglish);
                    if (Opt.AcceptTranslit.SafeTest(lang))
                        acc.Set(NLemmer::AccTranslit);

                    return acc;
                }

                bool ShouldReset_(ELanguage lang) const {
                    return Opt.ResetLemmaToForm && Opt.AcceptFoundling.SafeTest(lang);
                }
            };

            const TCoverage::ECoverageLevel TCoverage::CL_NONE = 0;
            const TCoverage::ECoverageLevel TCoverage::CL_PRIMARY = 32768;
            const TCoverage::ECoverageLevel TCoverage::CL_FOUNDLING = 1;
            const TCoverage::ECoverageLevel TCoverage::CL_BASTARD = 2;
            const TCoverage::ECoverageLevel TCoverage::CL_REGULAR = 3;
            const TCoverage::ECoverageLevel TCoverage::CL_PRIMARY_FOUNDLING = TCoverage::CL_FOUNDLING | TCoverage::CL_PRIMARY;
            const TCoverage::ECoverageLevel TCoverage::CL_PRIMARY_BASTARD = TCoverage::CL_BASTARD | TCoverage::CL_PRIMARY;
            const TCoverage::ECoverageLevel TCoverage::CL_PRIMARY_REGULAR = TCoverage::CL_REGULAR | TCoverage::CL_PRIMARY;

            template <class T>
            class TOffsetVector {
                typedef TVector<T> TBase;
                TBase& Base;
                const size_t Offset;

            public:
                TOffsetVector(TBase& base)
                    : Base(base)
                    , Offset(base.size())
                {
                }

                T& operator[](size_t i) {
                    return Base[Offset + i];
                }

                const T& operator[](size_t i) const {
                    return Base[Offset + i];
                }

                void push_back(const T& t) {
                    Base.push_back(t);
                }

                void pop_back() {
                    Base.pop_back();
                }

                T& back() {
                    return Base.back();
                }

                const T& back() const {
                    return Base.back();
                }

                size_t size() const {
                    Y_ASSERT(Base.size() >= Offset);
                    return Base.size() - Offset;
                }
            };

            using TLmOffsetVector = TOffsetVector<TYandexLemma>;
        }

        class TMultitokenGluerIterator;
        // Determine which separators are valid for the current language
        class TMultitokenGluer {
        private:
            const NLemmer::TClassifiedMultiToken& Multitoken;
            const ELanguage AlphaLangId;
            size_t Separators[MAX_SUBTOKENS + 2];
            size_t SeparatorsCount;

        public:
            TMultitokenGluer(const NLemmer::TClassifiedMultiToken& multitoken, const TLangAndMode& lam, TMultitokenOpt mopt)
                : Multitoken(multitoken)
                , AlphaLangId(lam.AlphaLangId())
                , SeparatorsCount(0)
            {
                Y_ASSERT(lam.Language());

                Separators[SeparatorsCount++] = 0;
                FillIntSeparators(lam, mopt);
                Separators[SeparatorsCount++] = Multitoken.Multitoken(AlphaLangId).SubTokens.size();
            }

            friend class TMultitokenGluerIterator;

        private:
            void FillIntSeparators(const TLangAndMode& lam, TMultitokenOpt mopt) {
                if (mopt == NLemmer::MtnWholly)
                    return;
                const TTokenStructure& tokens = Multitoken.Multitoken(AlphaLangId).SubTokens;
                const wchar16* str = Multitoken.Str(AlphaLangId);

                for (size_t i = 1; i < tokens.size(); i++) {
                    if (
                        // all means all
                        mopt == NLemmer::MtnSplitAll
                        // Never merge tokens that don't pertain to the current language
                        || !checkLanguage(i - 1) || !checkLanguage(i)
                        // Check if the space between prev and next is a separator
                        || lam.LangBreak()->CanBreak(str, tokens[i - 1].Pos, tokens[i - 1].Len, tokens[i].Pos, tokens[i].Len, lam.IsForeign())) {
                        Separators[SeparatorsCount++] = i;
                    }
                }
            }
            bool checkLanguage(size_t i) const {
                return AlphaLangId == LANG_UNK ||
                       Multitoken.TokenAlphaLanguages(i).Test(AlphaLangId);
            }
        };

        class TMultitokenGluerIterator {
            const TMultitokenGluer& Gluer;
            const size_t MaxTokensInCompound;
            size_t First;
            size_t Num;

        public:
            TMultitokenGluerIterator(const TMultitokenGluer& gluer, size_t maxTokensInCompound)
                : Gluer(gluer)
                , MaxTokensInCompound(maxTokensInCompound)
                , First(0)
                , Num(0)
            {
                Next();
            }

            bool IsValid() const {
                return First + 1 < Gluer.SeparatorsCount;
            }

            TSubTokenCoord operator*() const {
                Y_ASSERT(IsValid());
                return TSubTokenCoord(Begin(), End(), Num > 1);
            }

            bool Next() {
                // Take subchains containing no more than MAX_TOKENS_IN_COMPOUND tokens.
                // We count only pieces between valid separators, and only if all intermediate
                // tokens are eligible for the current language
                for (; First + 1 < Gluer.SeparatorsCount; ++First) {
                    if (!CheckAlpha(Begin()))
                        continue;

                    ++Num;
                    if (Num <= MaxTokensInCompound && First + Num < Gluer.SeparatorsCount) {
                        if (CheckAlpha(End() - 1))
                            return IsValid();
                    }
                    Num = 0;
                }
                return IsValid();
            }

        private:
            size_t Begin() const {
                return Gluer.Separators[First];
            }
            size_t End() const {
                return Gluer.Separators[First + Num];
            }
            bool CheckAlpha(size_t token) {
                return Gluer.Multitoken.TokenAlphaLanguages(token).SafeTest(Gluer.AlphaLangId);
            }
        };

        class TWordAnalyzer {
        private:
            const NLemmer::TClassifiedMultiToken& Multitoken;
            const NLemmer::TAnalyzeWordOpt& Opt;
            const TLangMask Langmask;
            const ELanguage* PriorLangs;
            TCoverage Coverage;

            TWLemmaArray& Out;

        public:
            TWordAnalyzer(TWLemmaArray& out, const NLemmer::TClassifiedMultiToken& multitoken, const NLemmer::TAnalyzeWordOpt& opt, const TLangMask& langmask, const ELanguage* priorLangs, EAnalyzeWordMode transMode)
                : Multitoken(multitoken)
                , Opt(opt)
                , Langmask(langmask)
                , PriorLangs(priorLangs)
                , Coverage(Multitoken.NumTokens(), Opt)
                , Out(out)
            {
                AnalyzeWord(transMode);
            }

        private:
            size_t AnalyzeWord(EAnalyzeWordMode transMode) {
                if (!Multitoken.Str() || !Multitoken.Length() || !Multitoken.NumTokens())
                    return 0;

                TLangMask langmaskLeft = Langmask;
                size_t count = AnalyzeWordPriorLangs(langmaskLeft, transMode);
                count += AnalyzeWordSecondaryLangs(langmaskLeft, transMode);

                if (!transMode) {
                    count += AddEssentialFoundlings<true>(LANG_UNK);
                    //Y_VERIFY (count > 0);
                }
                return count;
            }
            // analyze languages in the priority list
            size_t AnalyzeWordPriorLangs(TLangMask& langmaskLeft, EAnalyzeWordMode transMode) {
                if (!PriorLangs)
                    return 0;
                Coverage.SetPrimaryState(true);
                size_t count = 0;
                for (const ELanguage* idptr = PriorLangs; *idptr != LANG_UNK; idptr++) {
                    if (Langmask.Test(*idptr))
                        count += AnalyzeWordAsLanguage(TLangAndMode(*idptr, transMode), langmaskLeft);
                }

                return count;
            }

            // loop over all languages and add analyses
            size_t AnalyzeWordSecondaryLangs(TLangMask& langmaskLeft, EAnalyzeWordMode transMode) {
                Coverage.SetPrimaryState(false);
                size_t count = 0;
                for (ELanguage id : langmaskLeft) {
                    count += AnalyzeWordAsLanguage(TLangAndMode(id, transMode), langmaskLeft);
                }

                return count;
            }

            size_t AnalyzeWordAsLanguage(const TLangAndMode& lam, TLangMask& langmaskLeft) {
                size_t count = 0;
                if (lam.Language()) {
                    //TLangMask langmaskAlpha = langmaskLeft & Multitoken.CumulativeAlphaLanguage();
                    if (lam.IsForeign() || langmaskLeft.SafeTest(lam.Id()))
                        count += AnalyzeWordAsExistentLanguage(lam);
                } else {
                    if (Opt.ReturnFoundlingAnyway && !lam.IsForeign())
                        count += AddEssentialFoundlings<false>(lam.Id());
                }
                langmaskLeft.Reset(lam.Id()); // don't return to this language in the future
                return count;
            }

            template <bool uncoveredOnly>
            size_t AddEssentialFoundlings(ELanguage id) {
                NLemmer::TLanguageQualityAccept accept(NLemmer::AccFoundling);
                TLangAndMode lam(LANG_UNK, awModeNormal);
                const TWideToken& mt = Multitoken.Multitoken(id);

                size_t count = 0;
                if (Opt.Multitoken == NLemmer::MtnWholly) {
                    if (!uncoveredOnly || !Coverage.AnyCovered()) {
                        TSubTokenCoord sc(0, Multitoken.NumTokens(), false);
                        count += AnalyzeChunk(lam, accept, mt, sc);
                    }
                } else {
                    for (size_t i = 0; i < Multitoken.NumTokens(); i++) {
                        if (!uncoveredOnly || !Coverage.IsCovered(i))
                            count += AnalyzeChunk(lam, accept, mt, TSubTokenCoord(i));
                    }
                }

                for (size_t j = 0; j < count; j++) {
                    TYandexLemma& lm = Out[Out.size() - 1 - j];
                    NLemmerAux::TYandexLemmaSetter set(lm);
                    set.SetLanguage(id);
                    Coverage.AddLemma(lm);
                }

                return count;
            }

            // Provide analysis of a multitoken for a single language
            size_t AnalyzeWordAsExistentLanguage(const TLangAndMode& lam) {
                Y_ASSERT(lam.Language());

                TMultitokenGluer tokenGluer(Multitoken, lam, Opt.Multitoken);

                size_t count = 0;
                TLmOffsetVector offOut(Out);

                for (TMultitokenGluerIterator it(tokenGluer, Opt.MaxTokensInCompound); it.IsValid(); it.Next()) {
                    const TSubTokenCoord sc = *it;

                    NLemmer::TLanguageQualityAccept accept = Coverage.AcceptMode(lam, sc);
                    if (accept.none())
                        continue;
                    count += AnalyzeChunk(lam, accept, Multitoken.Multitoken(lam.AlphaLangId()), sc);
                }

                // Update the quality table
                for (size_t i = 0; i < count; i++)
                    Coverage.AddLemma(offOut[i]);

                for (size_t i = 0; i < count; i++) {
                    if (Coverage.ShouldReset(offOut[i]))
                        NLemmerAux::TYandexLemmaSetter(offOut[i]).ResetLemmaToForm();
                }

                if (Opt.AnalyzeWholeMultitoken && Multitoken.NumTokens() > 1) {
                    NLemmer::TLanguageQualityAccept accept;
                    accept.Set(NLemmer::AccDictionary);
                    accept.Set(NLemmer::AccBastard);
                    count += AnalyzeChunk(lam, accept, Multitoken.Multitoken(lam.AlphaLangId()), TSubTokenCoord(0, Multitoken.NumTokens(), true));
                }

                return count;
            }

            // Provide analysis for a single chunk
            size_t AnalyzeChunk(const TLangAndMode& lam, NLemmer::TLanguageQualityAccept accept,
                                const TWideToken& multitoken, const TSubTokenCoord& sc) {
                Y_ASSERT(lam.Language());
                TWtringBuf form = GetFormAndLen(multitoken, sc);

                TLmOffsetVector offOut(Out);

                size_t count = AnalyzeFormByMode(form, lam, accept);

                if (!count)
                    return count;
                // Add token composition info to the analyses
                AddLemmaInfo(offOut, sc);

                count += AddSuffix(multitoken, sc, offOut);

                return count;
            }

            size_t AnalyzeFormByMode(const TWtringBuf& form, const TLangAndMode& lam, NLemmer::TLanguageQualityAccept accept) {
                switch (lam.TransMode()) {
                    case awModeNormal: {
                        NLemmer::TRecognizeOpt recognizeOpt(Opt, accept);
                        recognizeOpt.SkipValidation = Opt.AnalyzeWholeMultitoken;
                        recognizeOpt.GenerateQuasiBastards = Opt.GenerateQuasiBastards.SafeTest(lam.Language()->Id);
                        lam.Language()->TuneOptions(Opt, recognizeOpt);
                        return lam.Language()->RecognizeAdd(form.data(), form.length(), Out, recognizeOpt);
                    }
                    case awModeTranslit:
                        if (accept.Test(NLemmer::AccTranslit)) {
                            NLemmer::TTranslitOpt t = Opt.GetTranslitOptions(lam.Language()->Id);
                            return lam.Language()->FromTranslitAdd(form.data(), form.length(), Out, Opt.MaxTranslitLemmas, &t, true);
                        }
                        break;
                    case awModeTranslate:
                        ythrow yexception() << "translate is not supported by lemmer";
                        break;
                }
                return 0;
            }

            void AddLemmaInfo(TLmOffsetVector& offOut, const TSubTokenCoord& sc) {
                const TWtringBuf formOrig = GetFormAndLen(Multitoken.Original(), sc);
                const TCharCategory charFlags = CaseFlags(sc);
                for (size_t j = 0; j < offOut.size(); j++) {
                    NLemmerAux::TYandexLemmaSetter set(offOut[j]);
                    set.SetToken(sc.Begin, sc.Length());
                    set.SetCaseFlags(charFlags);
                    set.SetInitialForm(formOrig.data(), formOrig.length());
                }
            }

            size_t AddSuffix(const TWideToken& multitoken, const TSubTokenCoord& sc, TLmOffsetVector& offOut) {
                if (sc.End != multitoken.SubTokens.size() || Opt.Suffix == NLemmer::SfxNo)
                    return 0;
                TWtringBuf suffix = CalcSuffix(multitoken, sc);
                if (!suffix.length())
                    return 0;
                return DoAddSuffix(suffix, offOut);
            }

            static TWtringBuf CalcSuffix(const TWideToken& multitoken, const TSubTokenCoord& sc) {
                TWtringBuf form = GetFormAndLen(multitoken, sc);
                size_t full_len = Min(multitoken.Leng - multitoken.SubTokens[sc.Begin].Pos, (size_t)TYandexLemma::InitialBufSize);
                if (full_len <= form.length())
                    return TWtringBuf();

                size_t length = full_len - form.length();
                const wchar16* suffix = form.data() + form.length();
                return TWtringBuf(suffix, length);
            }

            size_t DoAddSuffix(const TWtringBuf& suffix, TLmOffsetVector& offOut) {
                size_t count = 0;
                const size_t maxCount = offOut.size();
                for (size_t i = 0; i < maxCount; i++) {
                    if (Opt.Suffix == NLemmer::SfxBoth) {
                        offOut.push_back(offOut[i]);
                        if (NLemmerAux::TYandexLemmaSetter(offOut.back()).AddSuffix(suffix.data(), suffix.length()))
                            ++count;
                        else
                            offOut.pop_back();
                    } else {
                        Y_ASSERT(Opt.Suffix == NLemmer::SfxOnly);
                        NLemmerAux::TYandexLemmaSetter(offOut[i]).AddSuffix(suffix.data(), suffix.length());
                    }
                }
                return count;
            }

            static TWtringBuf GetFormAndLen(const TWideToken& multitoken, const TSubTokenCoord& sc) {
                const TTokenStructure& tokens = multitoken.SubTokens;
                const TCharSpan& first = tokens[sc.Begin];
                const TCharSpan& last = tokens[sc.End - 1];

                const wchar16* form = multitoken.Token + first.Pos;
                size_t len = last.EndPos() - first.Pos;
                Y_ASSERT(len <= multitoken.Leng);
                if (len > TYandexLemma::InitialBufSize)
                    len = TYandexLemma::InitialBufSize;
                return TWtringBuf(form, len);
            }

            TCharCategory CaseFlags(const TSubTokenCoord& sc) const {
                using NLemmerTokenNormalize::MergeCaseFlags;

                TCharCategory flags = MergeCaseFlags(
                    sc.Begin,
                    sc.End,
                    [this](size_t i) -> TCharCategory { return Multitoken.TokenAlphaCase(i); } //"this" should otlive lambda!
                );
                if (sc.IsMultitoken)
                    flags |= CC_COMPOUND;
                return flags;
            }
        };

    }

    size_t AnalyzeWord(const NLemmer::TClassifiedMultiToken& tokens, TWLemmaArray& out,
                       TLangMask langmask, const ELanguage* doclangs, const TAnalyzeWordOpt& opt) {
        out.clear();
        NDetail::TWordAnalyzer(out, tokens, opt, langmask, doclangs, NDetail::awModeNormal);
        if (opt.AcceptTranslit.any()) {
            NLemmer::TClassifiedMultiToken classifiedSecondary(tokens.Original(), false);
            NDetail::TWordAnalyzer(out, classifiedSecondary, opt, langmask, doclangs, NDetail::awModeTranslit);
        }
        return out.size();
    }

    size_t AnalyzeWord(const TWideToken& tokens, TWLemmaArray& out,
                       TLangMask langmask, const ELanguage* doclangs, const TAnalyzeWordOpt& opt) {
        //do nothing with empty token
        if (tokens.Leng == 0) {
            out.clear();
            return 0;
        }
        TClassifiedMultiToken mt(tokens);
        return AnalyzeWord(mt, out, langmask, doclangs, opt);
    }

}
