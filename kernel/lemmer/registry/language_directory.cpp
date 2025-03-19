#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/utility.h>
#include <util/generic/singleton.h>
#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/wide.h>
#include <kernel/lemmer/alpha/abc.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <util/string/split.h>

namespace NLemmer {
    // Auxiliary: table for language filtering by alphabet
    namespace {
        ////////////////////////////////////////////////////////////
        /////// TLanguageDirectoryImpl
        ///////////////////////////////////////////////////////////
        class TLanguageDirectoryImpl {
        private:
            // Index of all languages by ID
            TVector<TVector<const TLanguage*>> IndexById;
            // List of all known languages
            TLanguageVector KnownLanguages;
            TLangMask KnownLanguagesMask;

        public:
            TLanguageDirectoryImpl()
                : IndexById(LANG_MAX)
            {
            }

            const TLanguage* GetById(ELanguage id) const {
                if (id >= LANG_MAX)
                    return nullptr;
                return IndexById[id].empty() ? nullptr : IndexById[id][0];
            }

            const TLanguage* GetWithMeaningfulLemmas(ELanguage id) const {
                for (const auto& lang : IndexById[id]) {
                    if (lang->HasMeaningfulLemmas()) {
                        return lang;
                    }
                }
                return nullptr;
            }

            const TLanguage* GetByVersion(ELanguage id, int version) const {
                if (id >= LANG_MAX)
                    return nullptr;
                for (const TLanguage* lang : IndexById[id]) {
                    if (lang->GetVersion() == version) {
                        return lang;
                    }
                }
                return nullptr;
            }

            const TLanguage* GetPreviousVersion(ELanguage id, int version) const {
                if (id >= LANG_MAX)
                    return nullptr;
                const TLanguage* result = nullptr;
                for (const TLanguage* lang : IndexById[id]) {
                    if (lang->GetVersion() < version) {
                        if (!result || result->GetVersion() < lang->GetVersion()) {
                            result = lang;
                        }
                    }
                }
                return result;
            }

            const TLanguageVector& GetLanguageList() const {
                return KnownLanguages;
            }

            const TLangMask& GetLanguageMask() const {
                return KnownLanguagesMask;
            }

            void RegisterLanguage(const TLanguage* lang, bool forced) {
                Y_ASSERT(lang);
                if (!lang || lang->Id >= LANG_MAX) {
                    ythrow yexception() << "attempt to register bad language";
                }

                if (forced) {
                    IndexById[lang->Id].clear();
                }

                auto it = IndexById[lang->Id].begin();
                for (; it != IndexById[lang->Id].end() && (*it)->Version > lang->Version; ++it) {
                }
                if (it != IndexById[lang->Id].end() && (*it)->Version == lang->Version) {
                    return;
                }
                AddLang(lang);
                KnownLanguagesMask.SafeSet(lang->Id);
                IndexById[lang->Id].insert(it, lang);
            }

        private:
            void AddLang(const TLanguage* lang) {
                Y_ASSERT(lang);
                TLanguageVector::iterator it = KnownLanguages.begin();
                for (; it != KnownLanguages.end(); ++it) {
                    if ((*it)->Id >= lang->Id || (*it)->Id == lang->Id && (*it)->Version >= lang->Version)
                        break;
                }
                if (it != KnownLanguages.end() && (*it)->Id == lang->Id && (*it)->Version == lang->Version)
                    *it = lang;
                else
                    KnownLanguages.insert(it, lang);
            }
        };
    }

    const TLanguageDirectoryImpl* GetImpl() {
        return Singleton<TLanguageDirectoryImpl>();
    }

    const TLanguage* GetLanguageById(ELanguage id) {
        return GetImpl()->GetById(id);
    }

    const TLanguage* GetLanguageWithMeaningfulLemmasById(ELanguage id) {
        return GetImpl()->GetWithMeaningfulLemmas(id);
    }

    const TLanguage* GetLanguageByVersion(ELanguage id, int version) {
        return GetImpl()->GetByVersion(id, version);
    }

    const TLanguage* GetPreviousVersionOfLanguage(ELanguage id, int version) {
        return GetImpl()->GetPreviousVersion(id, version);
    }

    const TLanguage* GetLanguageByName(const char* name) {
        return GetLanguageById(LanguageByName(name));
    }

    const TLanguage* GetLanguageByName(const wchar16* name) {
        TWtringBuf s(name);
        return GetLanguageByName(WideToChar(s.data(), s.size(), CODES_YANDEX).c_str());
    }

    const TLanguageVector& GetLanguageList() {
        return GetImpl()->GetLanguageList();
    }

    TLangMask ClassifyLanguage(const wchar16* word, size_t len, bool ignoreDigit, bool primary) {
        return ClassifyLanguageAlpha(word, len, ignoreDigit, primary) & GetImpl()->GetLanguageMask();
    }

    TLangMask GetCharInfo(wchar16 ch, bool primary) {
        return GetCharInfoAlpha(ch, primary) & GetImpl()->GetLanguageMask();
    }

    void PreInitLanguages(TLangMask langMask /* = ~TLangMask()*/) {
        for (auto lang : GetLanguageList()) {
            assert(lang);
            if (!langMask.SafeTest(lang->Id))
                continue;
            TWLemmaArray out;
            TUtf16String latA = u"aaaaaaaaaaaaaaaaaaaaaa"; // latin
            TUtf16String rusA = u"аааааааааааааааааааааа";  // cyrillic
            lang->Recognize(latA.data(), 1, out);
            lang->Recognize(latA.data(), 2, out);
            lang->Recognize(latA.data(), 3, out);
            lang->Recognize(latA.data(), 16, out);

            lang->Recognize(rusA.data(), 1, out);
            lang->Recognize(rusA.data(), 2, out);
            lang->Recognize(rusA.data(), 3, out);
            lang->Recognize(rusA.data(), 16, out);
        }
    }

    TLangMask GetLanguagesMask(const TString& str, const TString& delimeters) {
        TLangMask res;

        TVector<TString> temp;
        StringSplitter(str).SplitBySet(delimeters.data()).SkipEmpty().Collect(&temp);

        for (const auto& i : temp) {
            const TLanguage* lang = GetLanguageByName(i.c_str());
            if (lang != nullptr) {
                res.SafeSet(lang->Id);
            }
        }

        return res;
    }

}

void TLanguage::RegisterLanguage(bool forced) const {
    const_cast<NLemmer::TLanguageDirectoryImpl*>(NLemmer::GetImpl())->RegisterLanguage(this, forced);
}
