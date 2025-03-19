#include "directory.h"
#include "abc.h"

#include <library/cpp/token/charfilter.h>

namespace NLemmer {
    // Auxiliary: table for language filtering by alphabet
    namespace NAlphaDirectory {
        ////////////////////////////////////////////////////////////
        /////// TAlphaDirectoryImpl
        ///////////////////////////////////////////////////////////

        const TAlphabetWordNormalizer* TAlphaDirectoryImpl::GetById(ELanguage id, bool primary) const {
            if (id >= LANG_MAX) {
                return PrimaryIndexById[LANG_UNK];
            }
            return (primary ? PrimaryIndexById : SecondaryIndexById)[id];
        }

        TLangMask TAlphaDirectoryImpl::GetWideCharInfo(wchar32 wch, bool primary) const {
            return GetGuessData(primary)->AbcTable.GetWideCharInfo(static_cast<wchar16>(wch));
        }

        TLangMask TAlphaDirectoryImpl::GetWideCharInfoStrict(wchar32 wch, bool primary) const {
            return GetGuessData(primary)->AbcStrictAlpha.GetWideCharInfo(static_cast<wchar16>(wch));
        }

        TLangMask TAlphaDirectoryImpl::GetWideCharInfoRequired(wchar32 wch, bool primary) const {
            return GetGuessData(primary)->AbcRequiredAlpha.GetWideCharInfo(static_cast<wchar16>(wch));
        }

        TLangMask TAlphaDirectoryImpl::GetWideCharInfoNormal(wchar32 wch, bool primary) const {
            return GetGuessData(primary)->AbcNormalAlpha.GetWideCharInfo(static_cast<wchar16>(wch));
        }

        void TAlphaDirectoryImpl::Register(const TAlphabetWordNormalizer* alphabet, ELanguage id, const TAlphabetWordNormalizer* secondary) {
            Register(alphabet, TLangMask(id), secondary);
        }

        void TAlphaDirectoryImpl::Register(const TAlphabetWordNormalizer* alphabet, const TLangMask& id, const TAlphabetWordNormalizer* secondary) {
            Register(alphabet, TLangMask(id), true);
            if (secondary) {
                Register(secondary, TLangMask(id), false);
            }
        }

        void TAlphaDirectoryImpl::Register(const TAlphabetWordNormalizer* alphabet, const TLangMask& mask, bool primary) {
            Y_ASSERT(alphabet);

            TIndex* index;
            TGuessData* guessData;
            if (primary) {
                index = &PrimaryIndexById;
                guessData = &PrimaryGuess;
            } else {
                index = &SecondaryIndexById;
                guessData = &SecondaryGuess;
            }

            if (!mask.any()) {
                Y_VERIFY(!PrimaryIndexById[LANG_UNK]);
                PrimaryIndexById[LANG_UNK] = alphabet;
                return;
            }

            for (ELanguage lg : mask) {
                Y_VERIFY(!(*index)[lg]);
                (*index)[lg] = alphabet;
            }

            for (wchar16 alpha = 65535; alpha > 0; --alpha) {
                TAlphabet::TCharClass c = alphabet->GetCharClass(alpha);
                if (c) {
                    guessData->AbcTable.AddSymbol(mask, alpha);
                    if (c & (TAlphabet::CharAlphaNormal | TAlphabet::CharAlphaNormalDec) && ((c & TAlphabet::CharDia) == 0)) {
                        guessData->AbcNormalAlpha.AddSymbol(mask, alpha);
                        if (c & TAlphabet::CharAlphaNormal)
                            guessData->AbcStrictAlpha.AddSymbol(mask, alpha);
                    }
                    if (c & TAlphabet::CharAlphaRequired) {
                        guessData->AbcRequiredAlpha.AddSymbol(mask, alpha);
                    }
                }
            }
        }
    }

    using NLemmer::NAlphaDirectory::TAlphaDirectoryImpl;

    static TAlphaDirectoryImpl* GetImpl() {
        return HugeSingleton<TAlphaDirectoryImpl>();
    }

    const NLemmer::TAlphabetWordNormalizer* GetAlphaRulesUnsafe(ELanguage id, bool primary) {
        return GetImpl()->GetById(id, primary);
    }

    TLangMask ClassifyLanguageAlpha(const wchar16* word, size_t len, bool ignoreDigit, bool primary) {
        const TAlphaDirectoryImpl* impl = GetImpl();
        TLangMask lang = ~TLangMask();
        TLangMask langNormal = TLangMask();
        bool any = false;
        for (size_t i = 0; i < len; i++) {
            if (ignoreDigit && IsDigit(word[i]))
                continue;
            lang &= impl->GetWideCharInfo(word[i], primary);
            langNormal |= impl->GetWideCharInfoNormal(word[i], primary);
            any = true;
        }
        if (!any)
            return TLangMask();
        // Don't apply Russian, Ukrainian, or Kazakh to renyxa words
        lang &= langNormal;

        return lang;
    }

    TLangMask GetCharInfoAlpha(wchar16 ch, bool primary) {
        return GetImpl()->GetWideCharInfoStrict(ch, primary);
    }

    TLangMask GetCharInfoAlphaRequired(wchar16 ch, bool primary) {
        return GetImpl()->GetWideCharInfoRequired(ch, primary);
    }

    //@todo: why it is in lemmer instead of something more general?
    TCharCategory ClassifyCase(const wchar16* word, size_t len) {
        TCharCategory flags = CC_WHOLEMASK & (~(CC_COMPOUND | CC_DIFFERENT_ALPHABET | CC_HAS_DIACRITIC));
        const wchar16* end = word + len;

        EScript prevScript = SCRIPT_UNKNOWN;
        for (const wchar16* p = word; p < end; p++) {
            if (*p < 128) //-- ASCII?
                flags &= ~CC_NONASCII;
            else
                flags &= ~CC_ASCII;

            if (IsLower(*p)) { //-- LOWER CASE?
                flags &= ~(CC_UPPERCASE | CC_NUMBER);
                if (p == word)
                    flags &= ~(CC_TITLECASE | CC_NUTOKEN);
            } else if (IsUpper(*p)) { //-- UPPER CASE?
                flags &= ~(CC_LOWERCASE | CC_NUMBER);
                if (p == word)
                    flags &= ~CC_NUTOKEN;
                else
                    flags &= ~CC_TITLECASE;
            } else if (IsDigit(*p)) { //-- DIGIT /* TREATED_AS_UPPERCASE */
                flags &= ~(CC_ALPHA | CC_LOWERCASE);
                if (p == word)
                    flags &= ~CC_NMTOKEN;
                else
                    flags &= ~CC_TITLECASE;
            } else if (IsAlphabetic(*p)) { //-- ALPHABET SYMBOL WITHOUT CASE
                flags &= ~(CC_NUMBER | CC_LOWERCASE | CC_UPPERCASE | CC_MIXEDCASE);
                if (p == word)
                    flags &= ~(CC_NUTOKEN | CC_TITLECASE);
            } else if (!IsCombining(*p)) { //-- NON ALPHANUMERIC /* TREATED_AS_LOWERCASE */
                flags &= ~(CC_ALPHA | CC_NUMBER | CC_NUTOKEN | CC_NMTOKEN | CC_LOWERCASE);
                if (p == word)
                    flags &= ~CC_TITLECASE;
            }

            EScript script = ScriptByGlyph(*p);
            if (script != SCRIPT_UNKNOWN) { // ignoring digits and other(?) strange symbols
                if (prevScript == SCRIPT_UNKNOWN)
                    prevScript = script;
                else if (prevScript != script)
                    flags |= CC_DIFFERENT_ALPHABET;
            }

            const wchar32* d = LemmerDecomposition(*p, false, true);
            if (d)
                flags |= CC_HAS_DIACRITIC;
        }

        //-- Single uppercase characters are treated as title case
        //-- Commented out: complicates treatment of compound tokens
        // if ((flags & CC_UPPERCASE) && (flags & CC_TITLECASE))
        //     flags &= ~CC_UPPERCASE;

        // Any specific case pattern turns mixed-case bit off
        if (flags & (CC_UPPERCASE | CC_LOWERCASE | CC_TITLECASE))
            flags &= ~CC_MIXEDCASE;

        return flags;
    }

}
