#include "blob_reader.h"

TBlobReader::TBlobReader(const TBlob& blob)
    : Blob(blob)
    , Read(0)
{
}

const ui8* TBlobReader::Cursor() const {
    return Blob.AsUnsignedCharPtr() + Read;
}

TBlob TBlobReader::Tail() const {
    return Blob.SubBlob(Read, Blob.Size());
}
