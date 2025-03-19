#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/ukr/main_with_trans/ukr.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char UkrDict[];
    extern const ui32 UkrDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(UkrDict, UkrDictSize)
        {
        }
    };

    class TUkrBuiltinLanguage: public TUkrLanguageWithTrans {
    public:
        TUkrBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TUkrBuiltinLanguage>();
}

void UseUkrBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
