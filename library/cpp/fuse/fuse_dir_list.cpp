#include "fuse_dir_list.h"

#include <arc/lib/util/compat.h>

#include <util/generic/yexception.h>

namespace NFuse {

bool TFuseDirList::Add(TStringBuf name, TInodeNum ino, ui32 type, off_t off) {
    Y_ASSERT(ino);
    const size_t entrySize = FUSE_DIRENT_ALIGN(FUSE_NAME_OFFSET + name.size());

    if (entrySize + Buf_.Size() > MaxSize_) {
        return false;
    }

    if (!Buf_.Capacity()) {
        Buf_.Reserve(MaxSize_);
    }

    struct fuse_dirent* const dirent = reinterpret_cast<fuse_dirent*>(Buf_.Pos());
    Buf_.Fill(0, FUSE_NAME_OFFSET);

    dirent->ino = ino;
    dirent->namelen = name.size();
    dirent->off = off;
    dirent->type = (S_IFMT & type) >> 12;

    Buf_.Append(name.data(), name.size());
    Buf_.Fill(0, entrySize - FUSE_NAME_OFFSET - name.size());

    return true;
}


TFuseDirListPlus::TEntry TFuseDirListPlus::Add(TStringBuf name, TInodeNum ino, ui32 type, off_t off) {
    Y_ASSERT(name);
    Y_ASSERT(ino);
    Y_ASSERT(type);
    Y_ASSERT(off);

#if FUSE_KERNEL_VERSION >= 7 && FUSE_KERNEL_MINOR_VERSION >= 21
    const size_t entrySize = FUSE_DIRENT_ALIGN(FUSE_NAME_OFFSET_DIRENTPLUS + name.size());
    TEntry res;

    if (entrySize + Buf_.Size() > MaxSize_) {
        return res;
    }

    if (!Buf_.Capacity()) {
        Buf_.Reserve(MaxSize_);
    }

    res.P_ = Buf_.Pos();
    struct fuse_direntplus* const dirent = reinterpret_cast<fuse_direntplus*>(Buf_.Pos());

    Buf_.Fill(0, FUSE_NAME_OFFSET_DIRENTPLUS);
    dirent->dirent.ino = ino;
    dirent->dirent.namelen = name.size();
    dirent->dirent.off = off;
    dirent->dirent.type = (S_IFMT & type) >> 12;

    Buf_.Append(name.data(), name.size());
    Buf_.Fill(0, entrySize - FUSE_NAME_OFFSET_DIRENTPLUS - name.size());

    return res;
#endif

    Y_UNUSED(name, off, MaxSize_);
    ythrow TSystemError(ENOSYS) << "READDIRPLUS is not supported";
}

void TFuseDirListPlus::TEntry::Fill(const fuse_entry_out& entry) {
#if FUSE_KERNEL_VERSION >= 7 && FUSE_KERNEL_MINOR_VERSION >= 21
    if (!P_) {
        Y_ASSERT(false);
        return;
    }

    struct fuse_direntplus* const dirent = reinterpret_cast<fuse_direntplus*>(P_);

    Y_ASSERT(dirent->dirent.namelen);
    Y_ASSERT(dirent->dirent.off);

    dirent->entry_out = entry;

    Y_ASSERT(dirent->dirent.ino == entry.nodeid);
    dirent->dirent.ino = entry.nodeid;

    Y_ASSERT(dirent->dirent.type == (entry.attr.mode >> 12));
    dirent->dirent.type = entry.attr.mode >> 12;

    return;
#endif

    Y_UNUSED(entry);
    ythrow TSystemError(ENOSYS) << "READDIRPLUS is not supported";

}

} // namespace NFuse
