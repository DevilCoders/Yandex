#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/por/main/por.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char PorDict[];
    extern const ui32 PorDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(PorDict, PorDictSize)
        {
        }
    };

    class TPorBuiltinLanguage: public TPorLanguage {
    public:
        TPorBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TPorBuiltinLanguage>();
}

void UsePorBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
