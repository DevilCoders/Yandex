#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/new_dict/rus/main_with_trans/rus.h>

namespace {
    extern "C" {
    extern const unsigned char RusExtraDict[];
    extern const ui32 RusExtraDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(RusExtraDict, RusExtraDictSize)
        {
        }
    };

    class TRusExtraLanguage: public TRusLanguageWithTrans {
    public:
        TRusExtraLanguage()
            : TRusLanguageWithTrans(LANG_RUS, 666)
        {
            Init(Singleton<TDict>()->GetBlob());
        }

        bool CheckOlderVersionsInRecognize(const wchar16*, size_t, const TWLemmaArray&, const NLemmer::TRecognizeOpt&) const override {
            return true;
        }

        TString DictionaryFingerprint() const override {
            const TLanguage* main = NLemmer::GetPreviousVersionOfLanguage(Id, Version);
            if (main) {
                return main->DictionaryFingerprint();
            }
            return ::TNewLanguage::DictionaryFingerprint();
        }
    };


    auto Lang = Singleton<TRusExtraLanguage>();
}

void UseRusExtraLanguage() {
    Y_UNUSED(Lang);
}
