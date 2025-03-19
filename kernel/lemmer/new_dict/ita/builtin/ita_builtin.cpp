#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/ita/main/ita.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char ItaDict[];
    extern const ui32 ItaDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(ItaDict, ItaDictSize)
        {
        }
    };

    class TItaBuiltinLanguage: public TItaLanguage {
    public:
        TItaBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TItaBuiltinLanguage>();
}

void UseItaBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
