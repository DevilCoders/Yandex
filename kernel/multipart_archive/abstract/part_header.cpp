#include "part_header.h"
#include "globals.h"

#include <util/ysaveload.h>
#include <util/stream/file.h>

#include <library/cpp/logger/global/global.h>

void NRTYArchive::IPartHeader::SaveRestoredHeader(const TFsPath& path, const TVector<ui32>& headerData) {
    VERIFY_WITH_LOG(!headerData.empty(), "Trying to dump empty header");

    TFileOutput out(path);
    ::SaveArray(&out, headerData.data(), headerData.size());
}
