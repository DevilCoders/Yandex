#include "archive.h"

namespace NRTYArchive {
    TVector<size_t> GetIdsFromSnapshot(TArrayRef<const TPosition> snapshot) {
        TVector<size_t> ids(Reserve(snapshot.size()));
        for (size_t id = 0; id != snapshot.size(); ++id) {
            if (!snapshot[id].IsRemoved()) {
                ids.push_back(id);
            }
        }
        return ids;
    }
}
