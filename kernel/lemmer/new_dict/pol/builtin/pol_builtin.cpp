#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/pol/main/pol.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char PolDict[];
    extern const ui32 PolDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(PolDict, PolDictSize)
        {
        }
    };

    class TPolBuiltinLanguage: public TPolLanguage {
    public:
        TPolBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TPolBuiltinLanguage>();
}

void UsePolBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
