#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/cze/main/cze.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char CzeDict[];
    extern const ui32 CzeDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(CzeDict, CzeDictSize)
        {
        }
    };

    class TCzeBuiltinLanguage: public TCzeLanguage {
    public:
        TCzeBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TCzeBuiltinLanguage>();
}

void UseCzeBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
