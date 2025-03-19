#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/bul/main/bul.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char BulDict[];
    extern const ui32 BulDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(BulDict, BulDictSize)
        {
        }
    };

    class TBulBuiltinLanguage: public TBulLanguage {
    public:
        TBulBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TBulBuiltinLanguage>();
}

void UseBulBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
