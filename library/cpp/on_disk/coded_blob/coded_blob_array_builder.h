#pragma once

#include "coded_blob_array.h"
#include "coded_blob_builder.h"

namespace NCodedBlob {
    class TCodedBlobArrayBuilder {
    public:
        void Init(IOutputStream* out, NCodecs::TCodecPtr trainedcodec);
        void Finish();

        void AddPrepared(TStringBuf prepareddata) {
            Offsets.push_back(DataBuilder.AddPrepared(prepareddata));
        }

        void AddCompressed(TStringBuf codeddata) {
            AddPrepared(NUtils::PrepareCompressedValueForBlob(codeddata, TmpBuf));
        }

    private:
        IOutputStream* Out = nullptr;
        TCodedBlobBuilder DataBuilder;
        TBuffer TmpBuf;
        TVector<ui64> Offsets;
    };

}
