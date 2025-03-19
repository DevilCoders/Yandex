#include <util/generic/buffer.h>
#include <yweb/protos/docfactors.pb.h>

#include "catalogreader.h"

TCatalogData::TCatalogData(const TBlob& blob)
#define GB(X) GetBlock(blob, X)
    : ManualAdultList(GB(0))
    , ManualNonAdultList(GB(1))
    , PornoMenuOwnersList(GB(2))
#undef GB
{
}

//////////////////////////////////////////////////////////////////////
TCatalogReader::TCatalogReader(const TString& filename) {
    Mf.Reset(new TMemoryMap(filename.data()));
    size_t len = IntegerCast<size_t>(Mf->Length());
    CatalogData.Reset(new TCatalogData(TBlob::FromMemoryMap(*Mf, 0, len)));
    ManualAdultList.Reset(new TBadUrlFilter(&CatalogData->ManualAdultList));
    ManualNonAdultList.Reset(new TBadUrlFilter(&CatalogData->ManualNonAdultList));
    PornoMenuOwnersList.Reset(new TBadUrlFilter(&CatalogData->PornoMenuOwnersList));
}

void TCatalogReader::FillAdultFactors(const char* pszUrl, NRealTime::TDocFactors& proto) const {
    proto.SetManualAdultness(ManualAdultList->CheckUrl(pszUrl));
    proto.SetManualNonAdultness(ManualNonAdultList->CheckUrl(pszUrl));
    proto.SetHasPornoMenu(PornoMenuOwnersList->CheckUrl(pszUrl));
}
