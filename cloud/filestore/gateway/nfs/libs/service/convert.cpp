#include "convert.h"

#include <cloud/filestore/libs/service/error.h>

#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

int ErrnoFromError(ui32 code)
{
    if (FACILITY_FROM_CODE(code) == FACILITY_SYSTEM) {
        return STATUS_FROM_CODE(code);
    }

    if (FACILITY_FROM_CODE(code) == FACILITY_FILESTORE) {
        return FileStoreErrorToErrno(STATUS_FROM_CODE(code));
    }

    return EIO;
}

void ConvertAttr(const NProto::TNodeAttr& attr, struct stat& st)
{
    Zero(st);

    st.st_ino = attr.GetId();

    st.st_mode = attr.GetMode() & ~S_IFMT;
    switch (attr.GetType()) {
        case NProto::E_DIRECTORY_NODE:
            st.st_mode |= S_IFDIR;
            break;
        case NProto::E_LINK_NODE:
            st.st_mode |= S_IFLNK;
            break;
        case NProto::E_REGULAR_NODE:
            st.st_mode |= S_IFREG;
            break;
    }

    st.st_uid = attr.GetUid();
    st.st_gid = attr.GetGid();
    st.st_size = attr.GetSize();
    // FIXME: number of actually allocated 512 blocks
    st.st_blocks = AlignUp<ui64>(st.st_size, 512) / 512;
    st.st_nlink = attr.GetLinks();
    st.st_atim = TimeSpecFromMicroSeconds(attr.GetATime());
    st.st_mtim = TimeSpecFromMicroSeconds(attr.GetMTime());
    st.st_ctim = TimeSpecFromMicroSeconds(attr.GetCTime());
}

void ConvertStat(const NProto::TFileStore& info, struct statfs& st)
{
    Zero(st);

    // Optimal transfer block size
    st.f_bsize = info.GetBlockSize();

    // Total data blocks in filesystem
    st.f_blocks= info.GetBlocksCount();

    // Free blocks in filesystem
    st.f_bfree = info.GetBlocksCount();

    // Free blocks available to unprivileged user
    st.f_bavail = info.GetBlocksCount();

    // Total inodes in filesystem
    st.f_files = Max();   // TODO

    // Free inodes in filesystem
    st.f_ffree = Max();   // TODO

    // Maximum length of filenames
    st.f_namelen = NProto::E_FS_LIMITS_NAME;

    // Fragment size (since Linux 2.6)
    st.f_frsize = info.GetBlockSize();
}

}   // namespace NCloud::NFileStore::NGateway
