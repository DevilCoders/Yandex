#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/tat/main/tat.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char TatDict[];
    extern const ui32 TatDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(TatDict, TatDictSize)
        {
        }
    };

    class TTatBuiltinLanguage: public TTatLanguage {
    public:
        TTatBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TTatBuiltinLanguage>();
}

void UseTatBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
