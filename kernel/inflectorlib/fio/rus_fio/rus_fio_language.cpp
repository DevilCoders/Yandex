#include "rus_fio_language.h"

#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
        extern const unsigned char RusFioDict[];
        extern const ui32 RusFioDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict(): TDictArchive(RusFioDict, RusFioDictSize) {}
    };
}

const TRusFIOLanguage* TRusFIOLanguage::GetLang() {
    return Singleton<TRusFIOLanguage>();
}

TRusFIOLanguage::TRusFIOLanguage()
    : TNewLanguage(LANG_RUS, true)
{
    Init(Singleton<TDict>()->GetBlob());
}

namespace {
    auto Lang = TRusFIOLanguage::GetLang();
}

void UseRusFIOLanguage() {
    // Fake use of Lang to fix `-Werror=unused-variable` whe LTO is enabled
    Y_UNUSED(Lang);
}
