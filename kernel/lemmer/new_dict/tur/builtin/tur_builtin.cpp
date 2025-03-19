#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/tur/main_with_trans/tur.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char TurDict[];
    extern const ui32 TurDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(TurDict, TurDictSize)
        {
        }
    };

    class TTurBuiltinLanguage: public TTurLanguageWithTrans {
    public:
        TTurBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TTurBuiltinLanguage>();
}

void UseTurBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
