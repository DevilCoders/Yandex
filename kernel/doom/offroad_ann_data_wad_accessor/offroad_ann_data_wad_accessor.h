#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>

#include <kernel/doom/wad/wad_index_type.h>
#include <kernel/indexann/interface/reader.h>

namespace NDoom {

class IWad;
class IDocLumpMapper;

THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(const IWad* wad);
THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(const IWad* wad, const IDocLumpMapper* mapper);
THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(const TString& path, bool lockMemory = false);

THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(EWadIndexType indexType, const IWad* wad, const IDocLumpMapper* mapper);
THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(EWadIndexType indexType, const TString& path, bool lockMemory = false);

} // namespace NDoom
