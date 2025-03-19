#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/kaz/main_with_trans/kaz.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char KazDict[];
    extern const ui32 KazDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(KazDict, KazDictSize)
        {
        }
    };

    class TKazBuiltinLanguage: public TKazLanguageWithTrans {
    public:
        TKazBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TKazBuiltinLanguage>();
}

void UseKazBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
