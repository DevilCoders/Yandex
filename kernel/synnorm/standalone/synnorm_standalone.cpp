#include "synnorm_standalone.h"

#include <library/cpp/archive/yarchive.h>

#include <util/generic/singleton.h>

extern "C" {
    extern const ui8 SynnormGztBinData[];
    extern const ui32 SynnormGztBinDataSize;
}

namespace {
    class TSynnormStandaloneIniter {
        NSynNorm::TSynNormalizer Data;
        void Init() {
            auto blob = TBlob::NoCopy(SynnormGztBinData, SynnormGztBinDataSize);

            Data.LoadStopwords(TArchiveReader{blob}.ObjectBlobByKey("/stopword.lst"));
            Data.LoadSynsets(TArchiveReader{blob}.ObjectBlobByKey("/synnorm.gzt.bin"));
        }
    public:
        TSynnormStandaloneIniter() {
            Init();
        }

        NSynNorm::TSynNormalizer& GetData() {
            return Data;
        }
    };


}

const  NSynNorm::TSynNormalizer& NSynNorm::GetStandAloneSynnormNormalizer()
{
    return Singleton<TSynnormStandaloneIniter>()->GetData();
}
