#pragma once

#include <kernel/tarc/iface/tarcio.h>
#include <ysite/yandex/common/mergemode.h>

class bitmap_2;

//! @attention in case of using remapping - text archive can be unsorted by docID
void MergeArchives(
    const char* suffix, const TVector<TString>& newNames, const TString& oldName, const TString& outName,
    MERGE_MODE defmode, const bitmap_2* addDelDocuments, ui32 delta, TArchiveVersion archiveVersion = ARCVERSION,
    const ui32* remap = nullptr, ui32 remapSize = 0
);
