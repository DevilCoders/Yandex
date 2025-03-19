#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/folder/path.h>

namespace NRTYArchive {
    constexpr TStringBuf PART_EXT = ".part.";
    constexpr TStringBuf PART_HEADER_EXT = ".hdr";
    constexpr TStringBuf PART_META_EXT = ".meta";
    constexpr TStringBuf FAT_EXT = ".fat";

    TFsPath GetFatPath(const TFsPath& name);
    TFsPath GetPartPath(const TFsPath& name, ui32 index);
    TFsPath GetPartHeaderPath(const TFsPath& name, ui32 index);
    TFsPath GetPartHeaderPath(const TFsPath& name);
    TFsPath GetPartMetaPath(const TFsPath& name, ui32 index);
    TFsPath GetPartMetaPath(const TFsPath& name);
}
