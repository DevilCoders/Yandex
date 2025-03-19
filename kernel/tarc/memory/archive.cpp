#include "archive.h"

#include <kernel/tarc/markup_zones/searcharc_common.h>

int IMemoryArchive::FindDoc(ui32 docId, TVector<int>& breaks, TDocArchive& da) const {
    TBlob b, doctext;
    try {
        GetExtInfoAndDocText(docId, b, doctext);
    } catch (...) {
        da.Clear();
        return -1;
    }
    return FindDocCommon(b, doctext, breaks, da);
}

int IMemoryArchive::FindDocBlob(ui32 docId, TDocArchive& da) const {
    TVector<int> breaks;
    return FindDoc(docId, breaks, da);
}
