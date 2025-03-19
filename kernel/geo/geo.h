#pragma once

#include <kernel/geo/tree/geotree.h>
#include "utils.h"

class TRegDBGeoTree: public TGeoTree
{
public:
    TRegDBGeoTree(const TGeoRegionDFSVector& regs, size_t defregion = 0)
    {
        for (TGeoRegionDFSVector::const_iterator it = regs.begin(); it != regs.end(); ++it) {
            const size_t child = static_cast<size_t>(it->first);
            const size_t parent = static_cast<size_t>(it->second >= 0 ? it->second : defregion);

            if (child == defregion && parent == defregion) {
                // avoid useless loop edge
                continue;
            }

            try {
                AddEdge(parent, child);
            } catch (const TMalformedException& e) {
                Cerr << "Malformed geo tree data: " << e.what() << ": child = " << child << ", parent = " << parent << Endl;
            }
        }
    }
};

