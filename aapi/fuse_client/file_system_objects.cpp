#include "file_system_objects.h"
#include "utils.h"

namespace NAapi {

bool TDirEntry::IsDir() const {
    return DirNode != nullptr;
}

bool TDirEntry::IsFile() const {
    return DirNode == nullptr;
}

TDirNode::TDirNode(time_t start, ui64 size)
    : Stat(DefaultModeStats(EEntryMode::EEM_DIR, start, size))
{
}

TDirNode::TDirNode(const TFileStat& st)
    : Stat(st)
{
}

int TDirNode::Chmod(mode_t mode) {
    if (!(mode & S_IFDIR)) {
        return -EPERM;
    }
    Stat.Mode = mode;
    return 0;
}

void TDirNode::Utimens(const struct timespec tv[2]) {
    if (Stat.ATime) {
        Stat.ATime = tv[0].tv_sec;
        Stat.MTime = tv[1].tv_sec;
    }
}

} // namespace NAapi
