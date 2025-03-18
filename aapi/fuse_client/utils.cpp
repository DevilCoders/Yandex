#include "utils.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

namespace NAapi {

#ifdef _darwin_
#   define lgetxattr(path, name, value, size) \
        (getxattr(path, name, value, size, 0, XATTR_NOFOLLOW))

#   define lsetxattr(path, name, value, size, flags) \
        (setxattr(path, name, value, size, 0, flags | XATTR_NOFOLLOW))

    int utimensat_impl(const char* path, const struct timespec tv[2]) {
        Y_UNUSED(path);
        Y_UNUSED(tv);
        return -1;
    }
#else
    int utimensat_impl(const char* path, const struct timespec tv[2]) {
        return ::utimensat(0, path, tv, AT_SYMLINK_NOFOLLOW);
    }
#endif

TFileStat CustomModeStats(mode_t mode, time_t start, ui64 size) {
    TFileStat st;

    st.Mode   =     mode;
    st.Uid    = getuid();
    st.Gid    = getgid();
    st.Size   =     size;
    st.ATime  =    start;
    st.MTime  =    start;
    st.CTime  =    start;
    st.NLinks =        1;

    if (mode & S_IFDIR) {
        st.NLinks = 2;
    }

    return st;
}

TFileStat DefaultModeStats(EEntryMode mode, time_t start, ui64 size) {
    switch (mode) {
        case (EEntryMode::EEM_DIR): {
            return CustomModeStats(S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH, start, size);
        }
        case (EEntryMode::EEM_LINK): {
            return CustomModeStats(S_IFLNK, start, size);
        }
        case (EEntryMode::EEM_EXEC): {
            return CustomModeStats(S_IFREG | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH, start, size);
        }
        case (EEntryMode::EEM_REG): {
            return CustomModeStats(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP, start, size);
        }
    }
}

} // namespace NAapi
