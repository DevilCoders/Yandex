#include "qeflexindex_1_impl.h"

const char TQEFlexIndex4Indexing::IImpl::Dummy[3] = {'\0'};

bool TQEFlexIndex4SearchInvRawBase::FindBlob(const char* rawKey, TBlob& blob, i32* elNum) const
{
    if (Skipper.Get() == nullptr)
         ythrow yexception() << "TQEFlexIndex4SearchInvRawBase needs skipper";

    TRequestContext rc;
    const YxRecord* rec = ExactSearch(&IndexDoc, rc, rawKey);
    if (!rec)
        return false;
    IndexDoc.GetBlob(blob, rec->Offset, rec->Length, RH_DEFAULT);

    if (elNum)
        *elNum = rec->Counter;

    return true;
}

