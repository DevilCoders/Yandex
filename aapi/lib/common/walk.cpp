#include "walk.h"

namespace NAapi {

TEntriesIter::TEntriesIter(const void* tree, EIterMode mode)
    : Tree(NNode::GetTree(tree))
    , IterMode(mode)
    , Cur(0)
{
    // Skip directory entries
    if (IterMode == EIterMode::EIM_FILES) {
        while (Cur < Tree->entries()->size() && static_cast<EEntryMode>(Tree->entries()->Get(Cur)->mode()) == EEntryMode::EEM_DIR) {
            ++Cur;
        }
    }
}

bool TEntriesIter::Next(TEntry& entry) {
    if (Cur == Tree->entries()->size()) {
        return false;
    }

    auto ent = Tree->entries()->Get(Cur++);
    EEntryMode mode = static_cast<EEntryMode>(ent->mode());

    if (IterMode == EIterMode::EIM_DIRS && mode != EEntryMode::EEM_DIR) {
        return false;
    }

    entry.Mode = mode;
    entry.Size = ent->size();
    entry.Name = TString(ent->name()->c_str());
    entry.Hash = TString(reinterpret_cast<const char*>(ent->hash()->Data()), ent->hash()->size());
    TVector<TString> blobs;
    for (size_t i = 0, size = ent->blobs()->size(), step = ent->hash()->size(); i < size; i += step) {
        blobs.emplace_back(reinterpret_cast<const char*>(ent->blobs()->Data()) + i, step);
    }
    entry.Blobs = std::move(blobs);

    return true;
}

ui64 TEntriesIter::Size() const {
    return Tree->entries()->size();
}

bool TEntriesIter::HasDirChild() const {
    return Tree->entries()->size() && static_cast<EEntryMode>(Tree->entries()->begin()->mode()) == EEntryMode::EEM_DIR;
}

}  // namespace NAapi
