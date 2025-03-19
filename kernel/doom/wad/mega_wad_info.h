#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>

#include "wad_lump_id.h"

class IInputStream;
class IOutputStream;
class TBlob;

namespace NDoom {


struct TMegaWadInfo {
    /** Mapping for global lumps. Lump type -> index in wad. */
    THashMap<TString, ui32> GlobalLumps;

    /** Per-document lumps, in document order. */
    TVector<TString> DocLumps;

    /** Index of the first document lump. */
    ui32 FirstDoc = 0;

    /** Total number of documents in megawad. */
    ui32 DocCount = 0;
};

void SaveMegaWadInfo(IOutputStream* output, const TMegaWadInfo& info);
TMegaWadInfo LoadMegaWadInfo(IInputStream* input);
TMegaWadInfo LoadMegaWadInfo(const TBlob& blob);


} // NDoom
