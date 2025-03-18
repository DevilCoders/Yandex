#pragma once

#include "coded_blob.h"

namespace NCodedBlob {
    class TCodedBlobBuilder : TNonCopyable {
    public:
        void Init(IOutputStream* out, NCodecs::TCodecPtr trainedcodec);
        void Finish();

        ui64 AddPrepared(TStringBuf prepareddata) {
            Out->Write(prepareddata.data(), prepareddata.size());

            ui64 offset = Offset;

            Offset += prepareddata.size();
            Count++;

            return offset;
        }

        ui64 AddCompressed(TStringBuf codeddata) {
            return AddPrepared(NUtils::PrepareCompressedValueForBlob(codeddata, TmpBuf));
        }

        ui64 GetOffset() const {
            return Offset;
        }

        ui64 GetCount() const {
            return Count;
        }

    private:
        NCodecs::TCodecPtr Codec;
        IOutputStream* Out = nullptr;
        ui64 Offset = 0;
        ui64 Count = 0;
        TBuffer TmpBuf;
    };

}
