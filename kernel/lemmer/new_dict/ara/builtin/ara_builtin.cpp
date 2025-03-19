#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/ara/main/ara.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char AraDict[];
    extern const ui32 AraDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(AraDict, AraDictSize)
        {
        }
    };

    class TAraBuiltinLanguage: public TAraLanguage {
    public:
        TAraBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TAraBuiltinLanguage>();
}

void UseAraBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
