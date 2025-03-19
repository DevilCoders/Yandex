#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/eng/basic/basic_eng.h>
#include <kernel/lemmer/new_dict/eng/main/eng.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char EngDict[];
    extern const ui32 EngDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(EngDict, EngDictSize)
        {
        }
    };

    class TEngBuiltinLanguage: public TEngLanguage {
    public:
        TEngBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    class TBasicEngBuiltinLanguage: public TBasicEngLanguage {
    public:
        TBasicEngBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TEngBuiltinLanguage>();
    auto Basic = Singleton<TBasicEngBuiltinLanguage>();
}

void UseEngBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
    Y_UNUSED(Basic);
}
