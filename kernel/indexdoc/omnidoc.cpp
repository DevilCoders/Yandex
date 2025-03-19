#include "omnidoc.h"
#include "omnidoc_fwd.h"
#include <kernel/doom/wad/wad.h>


TDocOmniWadIndex::~TDocOmniWadIndex() {
}

TDocOmniWadIndex::TDocOmniWadIndex(const NDoom::IWad* wad, const NDoom::IWad* l1wad, const NDoom::IWad* l2wad, const TString& oldIndexPath, bool useWadIndex)
    : TOldOmniIndexObsoleteDiePlease(oldIndexPath, !useWadIndex)
{
    if (wad || l2wad) {
        StringsWadSearchers_ = TStringsWadSearchers::InitHolders(l2wad ? l2wad : wad);
    }

    if (l1wad) {
        L1RawWadSearchers_ = TL1RawWadSearchers::InitHolders(l1wad);
    } else if (wad && wad->HasGlobalLump(NDoom::TWadLumpId(NDoom::OmniDssmEmbeddingType, NDoom::EWadLumpRole::StructModel))) {
        L1CompressedWadSearchers_ = TL1CompressedWadSearchers::InitHolders(wad);
    }

    if (l2wad) {
        L2RawWadSearchers_ = TL2RawWadSearchers::InitHolders(l2wad);
    } else if (wad) {
        L2CompressedWadSearchers_ = TL2CompressedWadSearchers::InitHolders(wad);
    }
}

TDocOmniWadIndex::TDocOmniWadIndex(const NDoom::IChunkedWad* l1wad, const NDoom::IChunkedWad* l2wad, const NDoom::IDocLumpMapper* mapper) {
    if (l1wad) {
        L1RawWadSearchers_ = TL1RawWadSearchers::InitHolders(l1wad, mapper);
    }

    if (l2wad) {
        StringsWadSearchers_ = TStringsWadSearchers::InitHolders(l2wad, mapper);
        L2RawWadSearchers_ = TL2RawWadSearchers::InitHolders(l2wad, mapper);
    }
}

size_t TDocOmniWadIndex::Size() const {
    return UseOmniLegacy_ ? TOldOmniIndexObsoleteDiePlease::Size() : std::get<0>(StringsWadSearchers_)->Size();
}
