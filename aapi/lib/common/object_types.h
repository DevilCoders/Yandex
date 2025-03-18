#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NAapi {

enum class EObjectType {
    EOT_BLOB = 0,
    EOT_TREE = 1,
    EOT_SVN_CI = 2,
    EOT_HG_CHANGESET = 3,
    EOT_HG_HEADS = 4,
    EOT_NAME = 100
};

enum class EEntryMode {
    EEM_REG = 0,
    EEM_EXEC = 1,
    EEM_LINK = 2,
    EEM_DIR = 3
};

struct TSvnCommitInfo {
    ui64 Revision;
    TString Author;
    TString Date;
    TString Message;
    TString TreeHash;
    TString ParentHash;
};

struct THgChangesetInfo {
    TString Hash;
    TString Author;
    TString Date;
    TString Message;
    TString Branch;
    TString Extra;
    TString TreeHash;
    TVector<TString> Parents;
    bool ClosesBranch;
};

struct TPathInfo {
    TString Path;
    EEntryMode Mode;
    TString Hash;
    TVector<TString> Blobs;
};

enum class EDiffStatus {
    EDS_MODIFIED = 0,
    EDS_CHANGED_FILE_MODE = 1,
    EDS_CHANGED_FILE_MODE_AND_MODIFIED = 2,
    EDS_ADDED = 3,
    EDS_DELETED = 4
};

struct TChangedPathInfo {
    TChangedPathInfo(const TString& path, EEntryMode mode, EDiffStatus status)
        : Path(path)
        , Mode(mode)
        , Status(status)
    {
    }

    TString Path;
    EEntryMode Mode;
    EDiffStatus Status;
};

}  // namespace NAapi
