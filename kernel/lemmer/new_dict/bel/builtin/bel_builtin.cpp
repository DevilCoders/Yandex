#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/bel/main/bel.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char BelDict[];
    extern const ui32 BelDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(BelDict, BelDictSize)
        {
        }
    };

    class TBelBuiltinLanguage: public TBelLanguage {
    public:
        TBelBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TBelBuiltinLanguage>();
}

void UseBelBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
