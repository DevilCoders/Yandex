#include <kernel/lemmer/new_dict/builtin/dict_archive.h>
#include <kernel/lemmer/new_dict/arm/main/arm.h>
#include <util/generic/singleton.h>

namespace {
    extern "C" {
    extern const unsigned char ArmDict[];
    extern const ui32 ArmDictSize;
    };

    class TDict: public TDictArchive {
    public:
        TDict()
            : TDictArchive(ArmDict, ArmDictSize)
        {
        }
    };

    class TArmBuiltinLanguage: public TArmLanguage {
    public:
        TArmBuiltinLanguage() {
            Init(Singleton<TDict>()->GetBlob());
        }
    };

    auto Lang = Singleton<TArmBuiltinLanguage>();
}

void UseArmBuiltinLanguage() {
    // spike for LTO, see SEARCH-1154
    Y_UNUSED(Lang);
}
