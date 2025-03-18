#include "head_ar.h"

#include <util/generic/buffer.h>

namespace NHeadAr {
    template <typename T>
    inline bool CheckedLoad(IInputStream* in, T& result) {
        return (in->Load(&result, sizeof(result)) == sizeof(result));
    }

    template <typename T>
    inline void Write(IOutputStream* out, const T& var) {
        out->Write(&var, sizeof(var));
    }

    inline bool CheckedBufLoad(IInputStream* in, char* buffer, size_t expectedSize) {
        return (in->Load(buffer, expectedSize) == expectedSize);
    }

}

void GetArrayHead(IInputStream* in, ui32* size, ui32* version, ui32* time, TBlob* genericData) {
    using namespace NHeadAr;

    ui32 sizeRead = 0;
    ui32 versionRead = 0;
    ui32 timeRead = 0;
    ui32 genericDataSizeRead = 0;

    Y_ENSURE(
        CheckedLoad(in, sizeRead) &&
            CheckedLoad(in, versionRead) &&
            CheckedLoad(in, timeRead) &&
            CheckedLoad(in, genericDataSizeRead),
        "Bad headed array header. ");

    if (0x20202020 == versionRead) { //Four spaces (TString is filled with spaces by default)
        versionRead = 0;
        timeRead = 0;
        genericDataSizeRead = 0;
    }

    if (size)
        *size = sizeRead;
    if (version)
        *version = versionRead;
    if (time)
        *time = timeRead;

    if (genericDataSizeRead > 0) {
        if (genericDataSizeRead > TArrayWithHeadBase::N_HEAD_SIZE - 4 * sizeof(ui32))
            ythrow yexception() << "Bad generic data size: " << genericDataSizeRead;
        TBuffer genericDataHolder(genericDataSizeRead);
        Y_ENSURE(CheckedBufLoad(in, genericDataHolder.Data(), genericDataSizeRead),
                 "Bad headed array header, generic data size mismatch. ");

        if (genericData)
            *genericData = TBlob::Copy(genericDataHolder.Data(), genericDataSizeRead);
    }

    const size_t skipSize = TArrayWithHeadBase::N_HEAD_SIZE - 4 * sizeof(ui32) - genericDataSizeRead;
    TBuffer dummy(skipSize);
    Y_ENSURE(CheckedBufLoad(in, dummy.Data(), skipSize), "Bad headed array header, skipsize mismatch. ");
}

ui64 GetHeadArRecordNum(const TString& filename) {
    ui32 size = 0;
    {
        ui32 time = 0;
        ui32 version = 0;
        TFileInput fIn(filename);
        GetArrayHead(&fIn, &size, &version, &time);
    }
    // on zero size we definitely get FPE
    Y_ENSURE(size, "Invalid headed array record size. ");
    const ui64 space = TFile(filename, RdOnly).GetLength() - TArrayWithHeadBase::N_HEAD_SIZE;
    Y_ENSURE(0 == space % size,
             "Bad array size (fractional record count): space " << space << " should be divisible by " << size << ". ");
    return space / size;
}

void ArrayWithHeadInit(IOutputStream* out, ui32 size, ui32 version, ui32 time, const TBlob& genericData) {
    using namespace NHeadAr;

    Write(out, size);
    Write(out, version);
    Write(out, time);

    ui32 genericDataSize = genericData.Size();
    Write(out, genericDataSize);
    if (genericDataSize > 0)
        out->Write(genericData.Data(), genericData.Size());

    size_t len = TArrayWithHeadBase::N_HEAD_SIZE - 4 * sizeof(ui32) - genericDataSize;
    TString fakeStr(len, 0); //len has type size_t, so if we omit second argument the string would be filled with spaces
    out->Write(fakeStr.data(), len);
}
