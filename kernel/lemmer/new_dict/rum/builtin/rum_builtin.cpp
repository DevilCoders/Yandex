#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/rum/main/rum.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char RumDict[];
    extern const ui32 RumDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(RumDict, RumDictSize)
        {
        }
    };

    class TRumBuiltinLanguage: public TRumLanguage {
    public:
        TRumBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TRumBuiltinLanguage>();
}

void UseRumBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
