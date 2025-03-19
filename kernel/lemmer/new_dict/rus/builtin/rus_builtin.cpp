#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/rus/basic/basic_rus.h>
#include <kernel/lemmer/new_dict/rus/main_with_trans/rus.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char RusDict[];
    extern const ui32 RusDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(RusDict, RusDictSize)
        {
        }
    };

    class TRusBuiltinLanguage: public TRusLanguageWithTrans {
    public:
        TRusBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    class TBasicRusBuiltinLanguage: public TBasicRusLanguage {
    public:
        TBasicRusBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TRusBuiltinLanguage>();
    auto Basic = Singleton<TBasicRusBuiltinLanguage>();
}

void UseRusBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
    Y_UNUSED(Basic);
}
