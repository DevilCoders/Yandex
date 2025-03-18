#pragma once

#include <library/cpp/hnsw/index/index_base.h>

#include <tools/idx_print/utils/options.h>

void PrintHnswIndex(const TIdxPrintOptions& options) {
    TBlob Data = TBlob::PrechargedFromFile(options.IndexPath);
    const ui32* data = reinterpret_cast<const ui32*>(Data.Begin());
    const ui32* const end = reinterpret_cast<const ui32*>(Data.End());

    ui32 numItems = *data++;
    const ui32 maxNeighbors = *data++;
    const ui32 levelSizeDecay = *data++;

    Y_ENSURE(levelSizeDecay > 0, "levelSizeDecay is 0");
    Cout << numItems << "\n";
    ui32 level = 0;
    for (; numItems > 1; numItems /= levelSizeDecay) {
        Y_ENSURE(data < end);
        Cout << "Level: " << level++ << "\n";
        ui32 neighbors = Min(maxNeighbors, numItems - 1);
        for (ui32 i = 0; i < numItems; ++i) {
            for (ui32 j = 0; j < neighbors; ) {
                Cout << *(data++) << " \t"[++j == neighbors];
            }
        }
        Cout << "\n";
    }
}
