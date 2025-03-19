#include "blob.h"

namespace NRemorphAPI {

namespace NImpl {

TBlob::TBlob(const ::TBlob& blob)
    : Blob(blob)
{
}

TBlob::TBlob(const TString& str)
    : Blob(::TBlob::FromString(str))
{
}

const char* TBlob::GetData() const {
    return Blob.AsCharPtr();
}

unsigned long TBlob::GetSize() const {
    return Blob.Size();
}

} // NImpl

} // NRemorphAPI
