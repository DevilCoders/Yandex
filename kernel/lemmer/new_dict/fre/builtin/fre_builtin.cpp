#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/fre/main/fre.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char FreDict[];
    extern const ui32 FreDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(FreDict, FreDictSize)
        {
        }
    };

    class TFreBuiltinLanguage: public TFreLanguage {
    public:
        TFreBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TFreBuiltinLanguage>();
}

void UseFreBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
