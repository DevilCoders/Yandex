#include "globals.h"

TFsPath NRTYArchive::GetFatPath(const TFsPath& name) {
    return ToString(name) + FAT_EXT;
}

TFsPath NRTYArchive::GetPartPath(const TFsPath& name, ui32 index) {
    return ToString(name) + PART_EXT + ToString(index);
}

TFsPath NRTYArchive::GetPartHeaderPath(const TFsPath& name) {
    return ToString(name) + PART_HEADER_EXT;
}

TFsPath NRTYArchive::GetPartMetaPath(const TFsPath & name, ui32 index) {
    return ToString(GetPartPath(name, index)) +  PART_META_EXT;
}

TFsPath NRTYArchive::GetPartMetaPath(const TFsPath& name) {
    return ToString(name) + PART_META_EXT;
}

TFsPath NRTYArchive::GetPartHeaderPath(const TFsPath& name, ui32 index) {
    return ToString(name) + PART_EXT + ToString(index) + PART_HEADER_EXT;
}
