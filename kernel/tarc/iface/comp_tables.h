#pragma once

#include <library/cpp/comptable/comptable.h>

#include <util/system/defaults.h>

namespace NArc {

NCompTable::TChunkCompressor&   GetMarkupZonesCompressor();
NCompTable::TChunkDecompressor& GetMarkupZonesDecompressor();
NCompTable::TChunkCompressor&   GetSentInfosCompressor();
NCompTable::TChunkDecompressor& GetSentInfosDecompressor();

const ui32 COMPR_MAGIC_NUMBER_V1 = 0xFFFFFF01;

} //NArc
