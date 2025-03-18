#pragma once

#include "minhash_func.h"

#include <util/system/defaults.h>
#include <util/stream/output.h>
#include <util/stream/zerocopy.h>
#include <util/generic/yexception.h>
#include <util/generic/string.h>

namespace NMinHash {
    class THashBuildException: public yexception {};

    class TChdHashBuilder {
    public:
        static const ui32 npos = static_cast<ui32>(-1);

    public:
        TChdHashBuilder(ui32 numKeys, double loadFactor = 0.99, ui32 keysPerBucket = 5, ui32 seed = 0, ui8 fprSize = 0);
        ~TChdHashBuilder();
        template <typename C>
        void Build(const C& cont, IOutputStream* out);
        template <typename C>
        TAutoPtr<TChdMinHashFunc> Build(const C& cont);

    private:
        class TImpl;
        TImpl* Impl_;
    };

    class TDistChdHashBuilder {
    public:
        TDistChdHashBuilder(ui32 numPortions, double loadFactor, ui32 keysPerBucket, ui8 fprSize, IOutputStream* out);
        template <typename C>
        void Add(const C& cont);
        void Add(IZeroCopyInput* inp, ui32 size);
        void Add(const TChdMinHashFunc& hash);
        void Finish();

    private:
        ui32 NumPortions_;
        double LoadFactor_;
        ui32 KeysPerBucket_;
        ui8 FprSize_;
        IOutputStream* Out_;
        TVector<ui64> Off_;
    };

    void CreateHash(const TString& fileName, double loadFactor, ui32 numKeys, ui32 keysPerBucket,
                    ui32 seed, ui8 errorBits, bool wide, IOutputStream* out);

    void CreateDistHash(const TString& inpFileName, const TString& outFileName, ui32 numPortions,
                        double loadFactor, ui32 keysPerBucket, ui8 errorBits, bool wide);

}
