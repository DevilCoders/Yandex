#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/spa/main/spa.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char SpaDict[];
    extern const ui32 SpaDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(SpaDict, SpaDictSize)
        {
        }
    };

    class TSpaBuiltinLanguage: public TSpaLanguage {
    public:
        TSpaBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TSpaBuiltinLanguage>();
}

void UseSpaBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
