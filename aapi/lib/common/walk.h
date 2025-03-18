#pragma once

#include "object_types.h"

#include <aapi/lib/node/tree.fbs.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/memory/blob.h>
#include <util/folder/path.h>

namespace NAapi {

struct TEntry {
    EEntryMode Mode;
    ui64 Size;
    TString Name;
    TString Hash;
    TVector<TString> Blobs;
};

class TEntriesIter {
public:
    enum class EIterMode {
        EIM_ALL,
        EIM_FILES,
        EIM_DIRS
    };

    TEntriesIter(const void* tree, EIterMode mode = EIterMode::EIM_ALL);
    bool Next(TEntry& entry);
    bool HasDirChild() const;
    ui64 Size() const;

private:
    const NNode::Tree* Tree;
    EIterMode IterMode;
    ui32 Cur;
};

}  // namespace NAapi

