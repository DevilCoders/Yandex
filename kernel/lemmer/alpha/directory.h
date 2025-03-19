#pragma once

#include "abc.h"

namespace NLemmer {
    namespace NAlphaDirectory {
        class TGuessTable {
        private:
            TLangMask widechartable[0x10000];

        public:
            TGuessTable() {
                memset(widechartable, 0, sizeof(widechartable));
            }

            void AddSymbol(const TLangMask& mask, wchar32 wch) {
                widechartable[(ui16)wch] |= mask;
            }

            TLangMask GetWideCharInfo(wchar16 wch) const {
                return widechartable[(ui16)wch];
            }
        };

        class TAlphaDirectoryImpl {
        private:
            // Index of all languages by ID
            using TIndex = const TAlphabetWordNormalizer* [LANG_MAX];
            TIndex PrimaryIndexById;
            //additional scripts - for translit
            TIndex SecondaryIndexById;
            // Alphabet-matching table
            struct TGuessData {
                TGuessTable AbcTable;
                TGuessTable AbcNormalAlpha;
                TGuessTable AbcStrictAlpha;
                TGuessTable AbcRequiredAlpha;
            };
            TGuessData PrimaryGuess; //main scripts
            TGuessData SecondaryGuess; //additional scripts - for translates

        public:
            TAlphaDirectoryImpl() {
                memset(PrimaryIndexById, 0, sizeof(PrimaryIndexById));
                memset(SecondaryIndexById, 0, sizeof(SecondaryIndexById));
                Init();
            }

            const TAlphabetWordNormalizer* GetById(ELanguage id, bool primary=true) const;

            TLangMask GetWideCharInfo(wchar32 wch, bool primary=true) const;

            TLangMask GetWideCharInfoStrict(wchar32 wch, bool primary=true) const;

            TLangMask GetWideCharInfoRequired(wchar32 wch, bool primary=true) const;

            TLangMask GetWideCharInfoNormal(wchar32 wch, bool primary=true) const;

        private:
            void Init();
            void Register(const TAlphabetWordNormalizer* primary, ELanguage id, const TAlphabetWordNormalizer* secondary=nullptr);

            void Register(const TAlphabetWordNormalizer* alphabet, const TLangMask& mask, const TAlphabetWordNormalizer* secondary=nullptr);
            void Register(const TAlphabetWordNormalizer* alphabet, const TLangMask& mask, bool primary);

            const TGuessData* GetGuessData(bool primary) const {
                return primary ? &PrimaryGuess : &SecondaryGuess;
            }
        };
    }
}
