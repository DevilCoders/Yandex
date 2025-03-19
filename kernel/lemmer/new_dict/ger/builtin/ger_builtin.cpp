#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/ger/main/ger.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char GerDict[];
    extern const ui32 GerDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(GerDict, GerDictSize)
        {
        }
    };

    class TGerBuiltinLanguage: public TGerLanguage {
    public:
        TGerBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TGerBuiltinLanguage>();
}

void UseGerBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
