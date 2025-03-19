#pragma once

#include <kernel/tarc/enums/searcharc.pb.h>
#include <util/generic/string.h>

using TArchiveVersion = ui32;
const TArchiveVersion ARCVERSION = 1;               // default archive version
const TArchiveVersion ARC_COMPRESSED_EXT_INFO = 2;  // archive with compressed extinfo field

enum EArchiveType {
    AT_FLAT,
    AT_MULTIPART
};

enum EArchiveOpenMode {
    AOM_FILE /* "File" */,
    AOM_MAP /* "MemoryMapped" */,
    AOM_BLOB /* "InMemory" */,
    AOM_LOCKED_BLOB /* "LockedInMemory" */,
    AOM_FILE_NO_REUSE /* "FileNoReuse" */,
    AOM_DIRECT_FILE /* "DirectFile" */,
};

EArchiveType ArcTypeFromProto(NArcProto::EArchiveType type);
TArchiveVersion ArcVersionFromProto(NArcProto::EArchiveVersion version);
